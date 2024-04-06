#ifndef _ASM_RISCV_GENESIS_H
#define _ASM_RISCV_GENESIS_H

/* Inner Kernel Section */
#define __genesis __section(".genesis.text")
#define __genesisdata __section(".genesis.data")

/***********************************************************/
/* Security-sensitive Operation                            */
/***********************************************************/
#define GENESIS_WRITE_SATP	0 // Supervisor Address Translation Protection
#define GENESIS_SET_PGD	1 // Set PGD
#define GENESIS_SET_P4D	2 // Set P4D
#define GENESIS_SET_PUD	3 // Set PUD
#define GENESIS_SET_PMD	4 // Set PMD
#define GENESIS_SET_PTE	5 // Set PTE
#define GENESIS_GET_AND_CLEAR_PTE		6 // Get/Clear PTE
#define GENESIS_TEST_AND_CLEAR_YOUNG_PTE	7 // Test and clear young pte
#define GENESIS_SET_WRPROTECT_PTE		8 // wrprotect PTE
#define GENESIS_ESTABLISH_PMD	9 // Establish PMD
#define GENESIS_WRITE_CSR	10 // Write CSR
#define GENESIS_SET_CSR	11 // Set CSR
#define GENESIS_COPY_USER	12 // Copy from/to user
#define GENESIS_CLEAR_USER	13 // Clear user
#define GENESIS_INIT_PGD	14 // Init PGD
#define GENESIS_INIT_P4D	15 // Init P4D
#define GENESIS_INIT_PUD	16 // Init PUD
#define GENESIS_INIT_PMD	17 // Init PMD
#define GENESIS_INIT_PTE	18 // Init PTE
#define GENESIS_WRITE_TVEC	19 // Set STVEC

#ifndef __ASSEMBLY__

/***********************************************************/
/* Init variables                                          */
/***********************************************************/
extern int genesis_enabled;

/***********************************************************/
/* The entry point                                         */
/***********************************************************/
unsigned long _genesis_entry(unsigned long svc_num,
			     unsigned long arg0,
			     unsigned long arg1);

unsigned long _genesis_uaccess_entry(unsigned long svc_num,
				     unsigned long arg0,
				     unsigned long arg1,
				     unsigned long arg2);

/***********************************************************/
/* Shadow Mapping                                          */
/***********************************************************/
extern unsigned long shadow_offset_base;
#define SHADOW_OFFSET shadow_offset_base

#define __phys_to_shadow(x) ((unsigned long)(x) + SHADOW_OFFSET)
#define __virt_to_shadow(x) (__pa(x) + SHADOW_OFFSET)

/***********************************************************/
/* ZONE_GENESIS (page tables)                              */
/***********************************************************/
#define GENESIS_ZONE_SZ 0x40000 // Reserve 1GB (0x40000 * 4K)

/***********************************************************/
/* MISC (debug)                                            */
/***********************************************************/
#define GENESIS_DEBUG 1

#endif /* __ASSEMBLY__ */

#endif /* _ASM_RISCV_GENESIS_H */
