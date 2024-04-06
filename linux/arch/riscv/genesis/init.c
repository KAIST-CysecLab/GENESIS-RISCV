#include <linux/init.h>
#include <linux/gfp.h>
#include <linux/memblock.h>

#include <asm/genesis.h>
#include <asm/vmlinux.lds.h>
#include <asm/page.h>
#include <asm/set_memory.h>

int genesis_enabled __ro_after_init = 0;

/* FIXME: Use a unused hole
 * Refer: Documentation/riscv/vm-layout.rst
 * */
#ifdef CONFIG_KASAN
#error "Disable KASAN to be compatible with GENESIS!"
#endif

unsigned long shadow_offset_base __ro_after_init = MODULES_LOWEST_VADDR - SZ_8G;
//unsigned long shadow_offset_base __ro_after_init = _AC(0xffffffc000000000, UL);

/* PRIVINST Section */
extern char __privinst_begin[], __privinst_end[];

/* GENESIS Inner kernel Section */
extern char __genesis_text_begin[], __genesis_text_end[];

#undef pr_fmt
#define pr_fmt(fmt) "[GENESIS] " fmt

void __init genesis_test(void)
{
	void *p, *p2;
	int *p3, *shadow_p3;

	pr_info("[GENESIS] TEST CODE START\n");
	pr_info("[GENESIS] TEST 1. GFP_GENESIS ");
	p = (void *)__get_free_page(GFP_KERNEL);
	pr_info("p1 va: 0x%px pa: 0x%lx (GFP_KERNEL)\n", p, __pa(p));
	p2 = (void *)__get_free_page(GFP_KERNEL);
	pr_info("p2 va: 0x%px pa: 0x%lx (GFP_KERNEL)\n", p2, __pa(p2));
	free_page((unsigned long int)p);
	free_page((unsigned long int)p2);

	p3 = (int *)__get_free_page(__GFP_GENESIS);
	pr_info("p3 va: 0x%px pa: 0x%lx (GFP_GENESIS)\n", p3, __pa(p3));

	pr_info("[GENESIS] TEST 2. SHADOW MAPPING ");
	pr_info("addr: %px, shadow_addr: %lx\n", p3, __virt_to_shadow(p3));
	*p3 = 1234; // okay
	shadow_p3 = (int *)__virt_to_shadow(p3);
	__enable_user_access();
	*shadow_p3 = 12345;
	pr_info("addr val: %d, shadow val: %d\n", *p3, *shadow_p3);
	__disable_user_access();
	free_page((unsigned long int)p3);

	pr_info("[GENESIS] TEST CODE END\n");
}

void __init genesis_zone_set_readonly(void)
{
	unsigned long base;
	int numpages;
	int ret;

	base = (unsigned long)__va((max_low_pfn - GENESIS_ZONE_SZ) << PAGE_SHIFT);
	numpages = GENESIS_ZONE_SZ;

#if (GENESIS_DEBUG)
	pr_info("[GENESIS] Mark GENESIS_ZONE as read-only "
		"0x%lx - 0x%lx\n", base, base + (numpages << PAGE_SHIFT));
#endif

	ret = set_memory_ro(base, numpages);
	if (ret)
		panic("[GENESIS] failed to mark readonly!");
}

void __init genesis_init(void)
{
	pr_info("TEXT BEGIN: %px, END: %px\n", __genesis_text_begin,
					       __genesis_text_end);

#if (GENESIS_DEBUG)
	genesis_test();
#endif

#if (0) // Disable in QEMU
	set_kernel_memory(__privinst_begin, __privinst_end,
			  set_memory_u_x);
#endif

	genesis_enabled = 1;

	genesis_zone_set_readonly();
}
