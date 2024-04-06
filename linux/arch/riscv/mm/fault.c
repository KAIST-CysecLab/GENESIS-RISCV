// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2009 Sunplus Core Technology Co., Ltd.
 *  Lennox Wu <lennox.wu@sunplusct.com>
 *  Chen Liqin <liqin.chen@sunplusct.com>
 * Copyright (C) 2012 Regents of the University of California
 */


#include <linux/mm.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/perf_event.h>
#include <linux/signal.h>
#include <linux/uaccess.h>
#include <linux/kprobes.h>
#include <linux/kfence.h>

#include <asm/ptrace.h>
#include <asm/tlbflush.h>
#include <asm/genesis.h>

#include "../kernel/head.h"

static void die_kernel_fault(const char *msg, unsigned long addr,
		struct pt_regs *regs)
{
	bust_spinlocks(1);

	pr_alert("Unable to handle kernel %s at virtual address " REG_FMT "\n", msg,
		addr);

	bust_spinlocks(0);
	die(regs, "Oops");
	make_task_dead(SIGKILL);
}

static inline void no_context(struct pt_regs *regs, unsigned long addr)
{
	const char *msg;

	/* Are we prepared to handle this kernel fault? */
	if (fixup_exception(regs))
		return;

	/*
	 * Oops. The kernel tried to access some bad page. We'll have to
	 * terminate things with extreme prejudice.
	 */
	if (addr < PAGE_SIZE)
		msg = "NULL pointer dereference";
	else {
		if (kfence_handle_page_fault(addr, regs->cause == EXC_STORE_PAGE_FAULT, regs))
			return;

		msg = "paging request";
	}

	die_kernel_fault(msg, addr, regs);
}

static inline void mm_fault_error(struct pt_regs *regs, unsigned long addr, vm_fault_t fault)
{
	if (fault & VM_FAULT_OOM) {
		/*
		 * We ran out of memory, call the OOM killer, and return the userspace
		 * (which will retry the fault, or kill us if we got oom-killed).
		 */
		if (!user_mode(regs)) {
			no_context(regs, addr);
			return;
		}
		pagefault_out_of_memory();
		return;
	} else if (fault & VM_FAULT_SIGBUS) {
		/* Kernel mode? Handle exceptions or die */
		if (!user_mode(regs)) {
			no_context(regs, addr);
			return;
		}
		do_trap(regs, SIGBUS, BUS_ADRERR, addr);
		return;
	}
	BUG();
}

static inline void bad_area(struct pt_regs *regs, struct mm_struct *mm, int code, unsigned long addr)
{
	/*
	 * Something tried to access memory that isn't in our memory map.
	 * Fix it, but check if it's kernel or user first.
	 */
	mmap_read_unlock(mm);
	/* User mode accesses just cause a SIGSEGV */
	if (user_mode(regs)) {
		do_trap(regs, SIGSEGV, code, addr);
		return;
	}

	no_context(regs, addr);
}

static inline void vmalloc_fault(struct pt_regs *regs, int code, unsigned long addr)
{
	pgd_t *pgd, *pgd_k;
	pud_t *pud_k;
	p4d_t *p4d_k;
	pmd_t *pmd_k;
	pte_t *pte_k;
	int index;
	unsigned long pfn;

	/* User mode accesses just cause a SIGSEGV */
	if (user_mode(regs))
		return do_trap(regs, SIGSEGV, code, addr);

	/*
	 * Synchronize this task's top level page-table
	 * with the 'reference' page table.
	 *
	 * Do _not_ use "tsk->active_mm->pgd" here.
	 * We might be inside an interrupt in the middle
	 * of a task switch.
	 */
	index = pgd_index(addr);
	pfn = csr_read(CSR_SATP) & SATP_PPN;
	pgd = (pgd_t *)pfn_to_virt(pfn) + index;
	pgd_k = init_mm.pgd + index;

	if (!pgd_present(*pgd_k)) {
		no_context(regs, addr);
		return;
	}
	set_pgd(pgd, *pgd_k);

	p4d_k = p4d_offset(pgd_k, addr);
	if (!p4d_present(*p4d_k)) {
		no_context(regs, addr);
		return;
	}

	pud_k = pud_offset(p4d_k, addr);
	if (!pud_present(*pud_k)) {
		no_context(regs, addr);
		return;
	}

	/*
	 * Since the vmalloc area is global, it is unnecessary
	 * to copy individual PTEs
	 */
	pmd_k = pmd_offset(pud_k, addr);
	if (!pmd_present(*pmd_k)) {
		no_context(regs, addr);
		return;
	}

	/*
	 * Make sure the actual PTE exists as well to
	 * catch kernel vmalloc-area accesses to non-mapped
	 * addresses. If we don't do this, this will just
	 * silently loop forever.
	 */
	pte_k = pte_offset_kernel(pmd_k, addr);
	if (!pte_present(*pte_k)) {
		no_context(regs, addr);
		return;
	}

	/*
	 * The kernel assumes that TLBs don't cache invalid
	 * entries, but in RISC-V, SFENCE.VMA specifies an
	 * ordering constraint, not a cache flush; it is
	 * necessary even after writing invalid entries.
	 */
	local_flush_tlb_page(addr);
}

static inline bool access_error(unsigned long cause, struct vm_area_struct *vma)
{
	switch (cause) {
	case EXC_INST_PAGE_FAULT:
		if (!(vma->vm_flags & VM_EXEC)) {
			return true;
		}
		break;
	case EXC_LOAD_PAGE_FAULT:
		if (!(vma->vm_flags & VM_READ)) {
			return true;
		}
		break;
	case EXC_STORE_PAGE_FAULT:
		if (!(vma->vm_flags & VM_WRITE)) {
			return true;
		}
		break;
	default:
		panic("%s: unhandled cause %lu", __func__, cause);
	}
	return false;
}

/*
 * This routine handles page faults.  It determines the address and the
 * problem, and then passes it off to one of the appropriate routines.
 */
asmlinkage void do_page_fault(struct pt_regs *regs)
{
	struct task_struct *tsk;
	struct vm_area_struct *vma;
	struct mm_struct *mm;
	unsigned long addr, cause;
	unsigned int flags = FAULT_FLAG_DEFAULT;
	int code = SEGV_MAPERR;
	vm_fault_t fault;

	cause = regs->cause;
	addr = regs->badaddr;

	tsk = current;
	mm = tsk->mm;

	if (kprobe_page_fault(regs, cause))
		return;

	/*
	 * Fault-in kernel-space virtual memory on-demand.
	 * The 'reference' page table is init_mm.pgd.
	 *
	 * NOTE! We MUST NOT take any locks for this case. We may
	 * be in an interrupt or a critical region, and should
	 * only copy the information from the master page table,
	 * nothing more.
	 */
	if (unlikely((addr >= VMALLOC_START) && (addr < VMALLOC_END))) {
		vmalloc_fault(regs, code, addr);
		return;
	}

#ifdef CONFIG_64BIT
	/*
	 * Modules in 64bit kernels lie in their own virtual region which is not
	 * in the vmalloc region, but dealing with page faults in this region
	 * or the vmalloc region amounts to doing the same thing: checking that
	 * the mapping exists in init_mm.pgd and updating user page table, so
	 * just use vmalloc_fault.
	 */
	if (unlikely(addr >= MODULES_VADDR && addr < MODULES_END)) {
		vmalloc_fault(regs, code, addr);
		return;
	}
#endif
	/* Enable interrupts if they were enabled in the parent context. */
	if (likely(regs->status & SR_PIE))
		local_irq_enable();

	/*
	 * If we're in an interrupt, have no user context, or are running
	 * in an atomic region, then we must not take the fault.
	 */
	if (unlikely(faulthandler_disabled() || !mm)) {
		tsk->thread.bad_cause = cause;
		no_context(regs, addr);
		return;
	}

	if (user_mode(regs))
		flags |= FAULT_FLAG_USER;

#ifdef CONFIG_GENESIS
	bool in_genesis(unsigned long addr);

	if (!user_mode(regs) && addr < TASK_SIZE &&
			in_genesis(instruction_pointer(regs)))
		die_kernel_fault("[GENESIS] Fault in the inner kernel\n",
				addr, regs);
#else
	if (!user_mode(regs) && addr < TASK_SIZE &&
			unlikely(!(regs->status & SR_SUM)))
		die_kernel_fault("access to user memory without uaccess routines",
				addr, regs);
#endif

	perf_sw_event(PERF_COUNT_SW_PAGE_FAULTS, 1, regs, addr);

	if (cause == EXC_STORE_PAGE_FAULT)
		flags |= FAULT_FLAG_WRITE;
	else if (cause == EXC_INST_PAGE_FAULT)
		flags |= FAULT_FLAG_INSTRUCTION;
retry:
	mmap_read_lock(mm);
	vma = find_vma(mm, addr);
	if (unlikely(!vma)) {
		tsk->thread.bad_cause = cause;
		bad_area(regs, mm, code, addr);
		return;
	}
	if (likely(vma->vm_start <= addr))
		goto good_area;
	if (unlikely(!(vma->vm_flags & VM_GROWSDOWN))) {
		tsk->thread.bad_cause = cause;
		bad_area(regs, mm, code, addr);
		return;
	}
	if (unlikely(expand_stack(vma, addr))) {
		tsk->thread.bad_cause = cause;
		bad_area(regs, mm, code, addr);
		return;
	}

	/*
	 * Ok, we have a good vm_area for this memory access, so
	 * we can handle it.
	 */
good_area:
	code = SEGV_ACCERR;

	if (unlikely(access_error(cause, vma))) {
		tsk->thread.bad_cause = cause;
		bad_area(regs, mm, code, addr);
		return;
	}

	/*
	 * If for any reason at all we could not handle the fault,
	 * make sure we exit gracefully rather than endlessly redo
	 * the fault.
	 */
	fault = handle_mm_fault(vma, addr, flags, regs);

	/*
	 * If we need to retry but a fatal signal is pending, handle the
	 * signal first. We do not need to release the mmap_lock because it
	 * would already be released in __lock_page_or_retry in mm/filemap.c.
	 */
	if (fault_signal_pending(fault, regs))
		return;

	if (unlikely(fault & VM_FAULT_RETRY)) {
		flags |= FAULT_FLAG_TRIED;

		/*
		 * No need to mmap_read_unlock(mm) as we would
		 * have already released it in __lock_page_or_retry
		 * in mm/filemap.c.
		 */
		goto retry;
	}

	mmap_read_unlock(mm);

	if (unlikely(fault & VM_FAULT_ERROR)) {
		tsk->thread.bad_cause = cause;
		mm_fault_error(regs, addr, fault);
		return;
	}
	return;
}
NOKPROBE_SYMBOL(do_page_fault);

#ifdef CONFIG_GENESIS

#undef pr_fmt
#define pr_fmt(fmt) "[GENESIS-FAULT-HANDLER] " fmt

extern char __genesis_text_begin[], __genesis_text_end;

inline bool in_genesis(unsigned long pc)
{
	if (pc >= (unsigned long)__genesis_text_begin
	    && pc < (unsigned long)__genesis_text_end)
		return true;
	return false;
}

static inline pte_t *lookup_pte(const struct mm_struct *mm,
				unsigned long addr)
{
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;

	pgd = pgd_offset(mm, addr);
	if (pgd_none(*pgd))
		return NULL;

	p4d = p4d_offset(pgd, addr);
	if (p4d_none(*p4d))
		return NULL;

	pud = pud_offset(p4d, addr);
	if (pud_none(*pud))
		return NULL;

	pmd = pmd_offset(pud, addr);
	if (pmd_none(*pmd))
		return NULL;

	return pte_offset_map(pmd, addr);
}

static inline int is_cow(pte_t pte, unsigned long flags)
{
	if ((flags & VM_WRITE) && !pte_write(pte)) // read-only
		return 1;
	return 0;
}

static int preprocess_one_page_fault(unsigned long addr, bool is_write)
{
	struct task_struct *tsk;
	struct vm_area_struct *vma;
	struct mm_struct *mm;
	unsigned int flags = FAULT_FLAG_DEFAULT;
	unsigned long cause = EXC_LOAD_PAGE_FAULT;
	vm_fault_t fault;
	pte_t *ptep;

	tsk = current;
	mm = tsk->mm;

	if (is_write) {
		flags |= FAULT_FLAG_WRITE;
		cause = EXC_STORE_PAGE_FAULT;
	}

	ptep = lookup_pte(mm, addr);
	if (ptep && pte_present(*ptep)) {
		if (!is_write)
			goto success;

		mmap_read_lock(mm);
		vma = find_vma(mm, addr);
		if (unlikely(!vma)) {
			mmap_read_unlock(mm);
			pr_err("Considered as un attack!\n");
			BUG();
		}

		if (!is_cow_mapping(vma->vm_flags)) {
			mmap_read_unlock(mm);
			goto success;
		}

		goto good_area;
	}

retry:
	mmap_read_lock(mm);

	vma = find_vma(mm, addr);
	if (unlikely(!vma)) {
		pr_info("find_vma Failed (%lx)\n", addr);
		mmap_read_unlock(mm);

		goto fail;
	}
	if (likely(vma->vm_start <= addr))
		goto good_area;
	if (unlikely(!(vma->vm_flags & VM_GROWSDOWN))) {
		pr_info("Not GROWSDOWN\n");
		mmap_read_unlock(mm);
		goto fail;
	}
	if (unlikely(expand_stack(vma, addr))) {
		pr_info("Expand_stack Failed\n");
		mmap_read_unlock(mm);
		goto fail;
	}

	/*
	 * Ok, we have a good vm_area for this memory access, so
	 * we can handle it.
	 */
good_area:
	/*
	 * If for any reason at all we could not handle the fault,
	 * make sure we exit gracefully rather than endlessly redo
	 * the fault.
	 */
	fault = handle_mm_fault(vma, addr, flags, NULL);

	if (unlikely(fault & VM_FAULT_RETRY)) {
		flags |= FAULT_FLAG_TRIED;

		/*
		 * No need to mmap_read_unlock(mm) as we would
		 * have already released it in __lock_page_or_retry
		 * in mm/filemap.c.
		 */
		goto retry;
	}

	mmap_read_unlock(mm);

	if (unlikely(fault & VM_FAULT_ERROR)) {
		pr_err("failed to handle a page fault!\n");
		goto fail;
	}

success:
	return 0;

fail:
#if (0)
	current->thread.bad_cause = cause;
	do_trap(task_pt_regs(current), SIGSEGV, SEGV_MAPERR, addr);
	pr_info("Killed the bad process\n");
#endif
	pr_info("Failed to prepare a page fault (%d)\n", current->pid);

	return -EFAULT;
}

int prepare_page_fault(void *to, const void *from, unsigned long len)
{
	void *tmp_to = (void *)ALIGN_DOWN((unsigned long)to, PAGE_SIZE);
	void *tmp_from = (void *)ALIGN_DOWN((unsigned long)from, PAGE_SIZE);

	if (to && (unsigned long)to < TASK_SIZE) { // if it is in user space
		do {
			if (preprocess_one_page_fault((unsigned long)tmp_to,
						  /*is_write*/ 1)) {
				return -EFAULT;
			}

			tmp_to += PAGE_SIZE;
		} while (tmp_to < to + len);
	}

	if (from && (unsigned long)from < TASK_SIZE) { // if it is in user space
		do {
			if (preprocess_one_page_fault((unsigned long)tmp_from,
						  /*is_write*/ 0)) {
				return -EFAULT;
			}

			tmp_from += PAGE_SIZE;
		} while (tmp_from < from + len);
	}

	return 0;
}
EXPORT_SYMBOL(prepare_page_fault);
#endif
