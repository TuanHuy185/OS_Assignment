/*
 * Copyright (C) 2024 pdnguyen of the HCMC University of Technology
 */
/*
 * Source Code License Grant: Authors hereby grants to Licensee 
 * a personal to use and modify the Licensed Source Code for 
 * the sole purpose of studying during attending the course CO2018.
 */
//#ifdef CPU_TLB
/*
 * CPU TLB
 * TLB module cpu/cpu-tlb.c
 */

#include "mm.h"
#include <stdlib.h>
#include <stdio.h>

#ifdef CPU_TLB
static int count_hit = 0 , count_miss = 0;

int tlb_change_all_page_tables_of(struct pcb_t *proc,  struct memphy_struct * mp) 
{
  /* TODO update all page table directory info 
   *      in flush or wipe TLB (if needed)
   */

  return 0;
}

int tlb_flush_tlb_of(struct pcb_t *proc, struct memphy_struct * mp) 
{
  /* TODO flush tlb cached*/

  return 0;
}

/*tlballoc - CPU TLB-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int tlballoc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
  int addr, val;

  /* By default using vmaid = 0 */
  val = __alloc(proc, 0, reg_index, size, &addr);

  /* TODO update TLB CACHED frame num of the new allocated page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/

  TLBMEMPHY_dump(proc->tlb);
  print_pgtbl(proc, 0, -1);

  return val;
}

/*pgfree - CPU TLB-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int tlbfree_data(struct pcb_t *proc, uint32_t reg_index)
{
  __free(proc, 0, reg_index);

  /* TODO update TLB CACHED frame num of freed page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/

  TLBMEMPHY_dump(proc->tlb);
  print_pgtbl(proc, 0, -1);
  
  return 0;
}


/*tlbread - CPU TLB-based read a region memory
 *@proc: Process executing the instruction
 *@source: index of source register
 *@offset: source address = [source] + [offset]
 *@destination: destination storage
 */
int tlbread(struct pcb_t * proc, uint32_t source,
            uint32_t offset, 	uint32_t destination) 
{
  BYTE data = -1;
  int frmnum = -1;
	
  /* TODO retrieve TLB CACHED frame num of accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
  /* frmnum is return value of tlb_cache_read/write value*/
  
  struct vm_rg_struct *currg = get_symrg_byid(proc->mm, source);
  if (currg == NULL) {
      printf("Invalid memory region ID\n");
      return -1;
  }
  if (check_if_in_freerg_list(proc, 0, currg) < 0) {
      printf("Read to freed region list\n");
      return -1;
  }
  int addr = currg->rg_start + offset;
    
  int pgnum = PAGING_PGN(addr);

  // Assuming tlb_cache_read returns -1 on failure
  if (tlb_cache_read(proc->tlb, proc->pid, pgnum, &frmnum) == -1) {
      printf("Failed to read TLB cache\n");
      return -1;
  }

#ifdef IODUMP
  if (frmnum >= 0){
    count_hit++;
    printf("TLB hit at read region=%d offset=%d\n", 
	         source, offset);
  }
  else {
    count_miss++;
    printf("TLB miss at read region=%d offset=%d\n", 
	         source, offset);
  }
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif

  if (frmnum >= 0) {
    int phyaddr = (frmnum << PAGING_ADDR_FPN_LOBIT) + offset;

    MEMPHY_read(proc->mram, phyaddr, &data);

    destination = (uint32_t) data;
    return 0;
  }

  int val = __read(proc, 0, source, offset, &data);
  destination = (uint32_t) data;

  /* TODO update TLB CACHED with frame num of recent accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/

  uint32_t pte = proc->mm->pgd[pgnum];
  frmnum = PAGING_FPN(pte);

  if (val == 0) {
      if (tlb_cache_write(proc->tlb, proc->pid, pgnum, frmnum) == -1) {
          printf("Failed to update TLB cache\n");
          return -1;
      }
  }

  TLBMEMPHY_dump(proc->tlb);
  print_pgtbl(proc, 0, -1);


  return val; 
} 

/*tlbwrite - CPU TLB-based write a region memory
 *@proc: Process executing the instruction
 *@data: data to be wrttien into memory
 *@destination: index of destination register
 *@offset: destination address = [destination] + [offset]
 */
int tlbwrite(struct pcb_t * proc, BYTE data,
             uint32_t destination, uint32_t offset)
{
  int val = -1;
  int frmnum = -1;

  /* TODO retrieve TLB CACHED frame num of accessing page(s))*/
  /* by using tlb_cache_read()/tlb_cache_write()
  frmnum is return value of tlb_cache_read/write value*/

  struct vm_rg_struct *currg = get_symrg_byid(proc->mm, destination);
  if (currg == NULL) {
      printf("Invalid memory region ID\n");
      return -1;
  }
  if (check_if_in_freerg_list(proc, 0, currg) < 0) {
      printf("Write to freed region list\n");
      return -1;
  }
  int addr = currg->rg_start + offset;

  int pgnum = PAGING_PGN(addr);

  tlb_cache_read(proc->tlb, proc->pid, pgnum, &frmnum);

#ifdef IODUMP
  if (frmnum >= 0) {
    count_hit++;
    printf("TLB hit at write region=%d offset=%d value=%d\n",
	          destination, offset, data);
  }
	else {
    count_miss++;
    printf("TLB miss at write region=%d offset=%d value=%d\n",
            destination, offset, data);
  }
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif

  if (frmnum >= 0) {
    int phyaddr = (frmnum << PAGING_ADDR_FPN_LOBIT) + offset;

    MEMPHY_write(proc->mram,phyaddr, val);

    return 0;
  }
  val = __write(proc, 0, destination, offset, data);

  /* TODO update TLB CACHED with frame num of recent accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/

  uint32_t pte = proc->mm->pgd[pgnum];
  frmnum = PAGING_FPN(pte);

  if (val == 0) {
    if (tlb_cache_write(proc->tlb, proc->pid, pgnum, frmnum) == -1) {
        printf("Failed to update TLB cache\n");
        return -1;
    }
  }

  TLBMEMPHY_dump(proc->tlb);
  print_pgtbl(proc, 0, -1);
  
  return val;
}

void print_TLB_performance() {
  printf("===================TLB-Performance===================\n");
  printf("TLB hit: %d times\n", count_hit);
  printf("TLB miss: %d times\n", count_miss);
  printf("TLB hit ratio: %d%%\n", count_hit * 100 / (count_hit + count_miss));
}
#endif
