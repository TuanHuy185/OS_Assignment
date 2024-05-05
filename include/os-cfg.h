#ifndef OSCFG_H
#define OSCFG_H

#define MLQ_SCHED 1
#define MAX_PRIO 140

#define CPU_TLB //tlb
//#define CPUTLB_FIXED_TLBSZ
#define TLB_FULLY_ASSOCIATE
//#define TLB_DIRECT_MAP
#define MM_PAGING //MMU //tlb
#define MM_FIXED_MEMSZ //sched //tlb
#define VMDBG 1
#define MMDBG 1
#define IODUMP 1
#define PAGETBL_DUMP 1

#endif
