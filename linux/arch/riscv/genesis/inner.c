#include <asm/genesis.h>
#include <asm/csr.h>
#include <asm/pgtable.h>

/* PRIVILEGED INSTRUCTIONS */
extern void _genesis_write_satp(unsigned long pgd);
extern void _genesis_write_csr(unsigned long arg0);
extern void _genesis_set_csr(unsigned long arg0);
extern void _genesis_write_tvec(unsigned long arg0);

/* COPY_TO/FROM USER */
extern unsigned long __genesis_asm_copy_user(unsigned long arg0,
					     unsigned long arg1,
					     unsigned long arg2);
extern unsigned long  __genesis_clear_user(unsigned long arg0,
					   unsigned long arg1);

/* PAGE TABLE OPERATIONS */
static inline void _genesis_set_pgd(pgd_t *pgdp, pgd_t pgd)
{
	pgd_t *spgdp;

	if (likely(genesis_enabled))
		spgdp = (pgd_t *)__virt_to_shadow(pgdp);
	else
		spgdp = pgdp;

	*spgdp = pgd;
}

static inline void _genesis_set_p4d(p4d_t *p4dp, p4d_t p4d)
{
	p4d_t *sp4dp;

	if (likely(genesis_enabled))
		sp4dp = (p4d_t *)__virt_to_shadow(p4dp);
	else
		sp4dp = p4dp;

	*sp4dp = p4d;
}

static inline void _genesis_set_pud(pud_t *pudp, pud_t pud)
{
	pud_t *spudp;

	if (likely(genesis_enabled))
		spudp = (pud_t *)__virt_to_shadow(pudp);
	else
		spudp = pudp;

	*spudp = pud;
}

static inline void _genesis_set_pmd(pmd_t *pmdp, pmd_t pmd)
{
	pmd_t *spmdp;

	if (likely(genesis_enabled))
		spmdp = (pmd_t *)__virt_to_shadow(pmdp);
	else
		spmdp = pmdp;

	*spmdp = pmd;
}

static inline void _genesis_set_pte(pte_t *ptep, pte_t pteval)
{
	pte_t *sptep;

	if (likely(genesis_enabled))
		sptep = (pte_t *)__virt_to_shadow(ptep);
	else
		sptep = ptep;

	*sptep = pteval;
}

static inline unsigned long _genesis_ptep_set_and_clear(pte_t *ptep)
{
	pte_t *sptep;

	if (likely(genesis_enabled))
		sptep = (pte_t *)__virt_to_shadow(ptep);
	else
		sptep = ptep;

	return atomic_long_xchg((atomic_long_t *)sptep, 0);
}

static inline int _genesis_ptep_test_and_clear_young(pte_t *ptep)
{
	pte_t *sptep;

	if (likely(genesis_enabled))
		sptep = (pte_t *)__virt_to_shadow(ptep);
	else
		sptep = ptep;

	return test_and_clear_bit(_PAGE_ACCESSED_OFFSET, &pte_val(*sptep));
}

static inline void _genesis_set_wrprotect(pte_t *ptep)
{
	pte_t *sptep;

	if (likely(genesis_enabled))
		sptep = (pte_t *)__virt_to_shadow(ptep);
	else
		sptep = ptep;

	atomic_long_and(~(unsigned long)_PAGE_WRITE, (atomic_long_t *)sptep);
}

/* INIT PGD */
static void __genesis __genesis_init_pgd(pgd_t *pgdp)
{
	pgd_t *spgdp;

	if (likely(genesis_enabled))
		spgdp = (pgd_t *)__virt_to_shadow(pgdp);
	else
		spgdp = pgdp;

	memset(spgdp, 0, USER_PTRS_PER_PGD * sizeof(pgd_t));
	/* Copy kernel mappings */
	memcpy(spgdp + USER_PTRS_PER_PGD,
	       init_mm.pgd + USER_PTRS_PER_PGD,
	       (PTRS_PER_PGD - USER_PTRS_PER_PGD) * sizeof(pgd_t));
}

/* INIT P4D, PUD, PMD, PTE (except PGD) */
static void __genesis __genesis_init_pgtbl(void *pgtbl)
{
	void *shadow_pgtbl;

	if (likely(genesis_enabled))
		shadow_pgtbl = (void *)__virt_to_shadow(pgtbl);
	else
		shadow_pgtbl = pgtbl;

	memset(shadow_pgtbl, 0, PAGE_SIZE);
}

unsigned long __genesis inner_handler(unsigned long svc_num,
				      unsigned long arg0,
				      unsigned long arg1,
				      unsigned long arg2,
				      unsigned long arg3)
{
	switch (svc_num)
	{
	/* Privileged operations */
	case GENESIS_WRITE_SATP:
		_genesis_write_satp(arg0);
		break;
	case GENESIS_WRITE_CSR:
		_genesis_write_csr(arg0);
		break;
	case GENESIS_SET_CSR:
		_genesis_set_csr(arg0);
		break;

	/* Page table modifications */
	case GENESIS_SET_PGD:
		_genesis_set_pgd((pgd_t *)arg0, __pgd(arg1));
		break;
	case GENESIS_SET_P4D:
		_genesis_set_p4d((p4d_t *)arg0, __p4d(arg1));
		break;
	case GENESIS_SET_PUD:
		_genesis_set_pud((pud_t *)arg0, __pud(arg1));
		break;
	case GENESIS_SET_PMD:
		_genesis_set_pmd((pmd_t *)arg0, __pmd(arg1));
		break;
	case GENESIS_SET_PTE:
		_genesis_set_pte((pte_t *)arg0, __pte(arg1));
		break;
	case GENESIS_GET_AND_CLEAR_PTE:
		return _genesis_ptep_set_and_clear((pte_t *)arg0);
	case GENESIS_TEST_AND_CLEAR_YOUNG_PTE:
		return _genesis_ptep_test_and_clear_young((pte_t *)arg0);
	case GENESIS_SET_WRPROTECT_PTE:
		_genesis_set_wrprotect((pte_t *)arg0);
		break;
	case GENESIS_ESTABLISH_PMD:
		BUG(); // XXX: Maybe this is used only with HUGEPAGE enabled
		break;

	/* COPY_FROM/TO_USER */
	case GENESIS_COPY_USER:
		return __genesis_asm_copy_user(arg0, arg1, arg2);
	case GENESIS_CLEAR_USER:
		return __genesis_clear_user(arg0, arg1);

	/* INIT PAGE TABLES  */
	case GENESIS_INIT_PGD:
		__genesis_init_pgd((pgd_t *)arg0);
		break;
	case GENESIS_INIT_P4D:
	case GENESIS_INIT_PUD:
	case GENESIS_INIT_PMD:
	case GENESIS_INIT_PTE:
		__genesis_init_pgtbl((void *)arg0);
		break;

	case GENESIS_WRITE_TVEC:
		// XXX: Currently, unconditionally disallow the change of VTEC
		// TODO: Be compatible with the system suspend
		if (genesis_enabled)
			BUG();
		_genesis_write_tvec(arg0);
		break;
	}

	return 0;
}
