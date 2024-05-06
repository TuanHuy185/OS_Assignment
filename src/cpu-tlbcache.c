/*
 * Copyright (C) 2024 pdnguyen of the HCMC University of Technology
 */
/*
 * Source Code License Grant: Authors hereby grants to Licensee 
 * a personal to use and modify the Licensed Source Code for 
 * the sole purpose of studying during attending the course CO2018.
 */
//#ifdef MM_TLB
/*
 * Memory physical based TLB Cache
 * TLB cache module tlb/tlbcache.c
 *
 * TLB cache is physically memory phy
 * supports random access 
 * and runs at high speed
 */

#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

/* TLB version 2
Each entries have 64 bits
   Tag:
      Bit 31: Valid
      Bit 30: Used
      Bit 29-14: PID 
      Bit 13-0: Page number
   Frame number: 32 bit
*/
static int NUM_ENTRY_TLB;

#ifdef TLB_FULLY_ASSOCIATE
// PID
#define TLB_TAG_PID_HIBIT 29
#define TLB_TAG_PID_LOBIT 14
// Page number
#define TLB_TAG_PGN_HIBIT 13
#define TLB_TAG_PGN_LOBIT 0
// TAG
#define TLB_TAG_VALID_MASK BIT(31)
#define TLB_TAG_USED_MASK BIT(30)
#define TLB_TAG_PID_MASK GENMASK(TLB_TAG_PID_HIBIT, TLB_TAG_PID_LOBIT)
#define TLB_TAG_PGN_MASK GENMASK(TLB_TAG_PGN_HIBIT, TLB_TAG_PGN_LOBIT)
// Extract
#define TLB_TAG_VALID(x) GETVAL(x, TLB_TAG_VALID_MASK, 31)
#define TLB_TAG_USED(x) GETVAL(x, TLB_TAG_USED_MASK, 30)
#define TLB_TAG_PID(x) GETVAL(x, TLB_TAG_PID_MASK, TLB_TAG_PID_LOBIT)
#define TLB_TAG_PGN(x) GETVAL(x, TLB_TAG_PGN_MASK, TLB_TAG_PGN_LOBIT)
#endif

#ifdef TLB_DIRECT_MAP
// PID
#define TLB_TAG_PID_HIBIT 30
#define TLB_TAG_PID_LOBIT 14
// Page number
#define TLB_TAG_PGN_HIBIT 13
#define TLB_TAG_PGN_LOBIT 0
// TAG
#define TLB_TAG_VALID_MASK BIT(31)
#define TLB_TAG_PID_MASK GENMASK(TLB_TAG_PID_HIBIT, TLB_TAG_PID_LOBIT)
#define TLB_TAG_PGN_MASK GENMASK(TLB_TAG_PGN_HIBIT, TLB_TAG_PGN_LOBIT)
// Extract
#define TLB_TAG_VALID(x) GETVAL(x, TLB_TAG_VALID_MASK, 31)
#define TLB_TAG_PID(x) GETVAL(x, TLB_TAG_PID_MASK, TLB_TAG_PID_LOBIT)
#define TLB_TAG_PGN(x) GETVAL(x, TLB_TAG_PGN_MASK, TLB_TAG_PGN_LOBIT)
#endif

#define init_tlbcache(mp,sz,...) init_memphy(mp, sz, (1, ##__VA_ARGS__))
static pthread_mutex_t tlb_lock;

int tlb_clear_bit_valid(struct memphy_struct *mp, int pid, int pgnum) {
    int result = -1; // Assume failure by default

    pthread_mutex_lock(&tlb_lock);

#ifdef TLB_FULLY_ASSOCIATE
    for (int i = 0; i < NUM_ENTRY_TLB; i++) {
        uint32_t *tag = (uint32_t *)(mp->storage + i * 8);

        int pid_TLB = TLB_TAG_PID(*tag);
        int pgnum_TLB = TLB_TAG_PGN(*tag);

        if (pid == pid_TLB && pgnum == pgnum_TLB) {
            CLRBIT(*tag, TLB_TAG_VALID_MASK);
            result = 0; // Success
            break;
        }
    }
#endif

#ifdef TLB_DIRECT_MAP
    int i = pgnum % NUM_ENTRY_TLB;
    uint32_t *tag = (uint32_t *)(mp->storage + i * 8);

    int pid_TLB = TLB_TAG_PID(*tag);
    int pgnum_TLB = TLB_TAG_PGN(*tag);

    if (pid == pid_TLB && pgnum == pgnum_TLB) {
        CLRBIT(*tag, TLB_TAG_VALID_MASK);
        result = 0; // Success
    }
#endif

    pthread_mutex_unlock(&tlb_lock);
    return result;
}

int tlb_flush_entry(struct memphy_struct *mp, int pid, int pgnum) {
   pthread_mutex_lock(&tlb_lock);
#ifdef TLB_FULLY_ASSOCIATE
   for (int i = 0; i < NUM_ENTRY_TLB; i++) {
      uint32_t *tag = (uint32_t*)(mp->storage + i*8);
      uint32_t *frmnum = (uint32_t*)(mp->storage + i*8 + 4);
      int pid_TLB = TLB_TAG_PID(*tag);
      int pgnum_TLB = TLB_TAG_PGN(*tag);
      if (pid == pid_TLB && pgnum == pgnum_TLB) {
         CLRBIT(*tag, TLB_TAG_VALID_MASK);
         CLRBIT(*tag, TLB_TAG_USED_MASK);
         SETVAL(*tag, 0, TLB_TAG_PID_MASK, TLB_TAG_PID_LOBIT);
         SETVAL(*tag, 0, TLB_TAG_PGN_MASK, TLB_TAG_PGN_LOBIT);
         *frmnum = 0;
         break;
      }
  }
#endif
  pthread_mutex_unlock(&tlb_lock);
  return 0;
}
/*
 *  tlb_cache_read read TLB cache device
 *  @mp: memphy struct
 *  @pid: process id
 *  @pgnum: page number
 *  @value: obtained value
 */
int tlb_cache_read(struct memphy_struct * mp, int pid, int pgnum, int *value)
{
   /* TODO: the identify info is mapped to 
    *      cache line by employing:
    *      direct mapped, associated mapping etc.
    */
   if (mp == NULL) return -1;
   
   // int NUM_ENTRY_TLB = mp->maxsz/10;
   pthread_mutex_lock(&tlb_lock);

   #ifdef TLB_FULLY_ASSOCIATE
   for (int i = 0; i < NUM_ENTRY_TLB; i++) {
      uint32_t *tag = (uint32_t*)(mp->storage + i*8);
      uint32_t *frmnum = (uint32_t*)(mp->storage + i*8 + 4);

      int valid = TLB_TAG_VALID(*tag);
      int pid_TLB = TLB_TAG_PID(*tag);
      int pgnum_TLB = TLB_TAG_PGN(*tag);

      if (pid_TLB == pid && valid && pgnum_TLB == pgnum) {
         *value = *frmnum;
         SETBIT(*tag, TLB_TAG_USED_MASK);
         pthread_mutex_unlock(&tlb_lock);
         return 0;
      }
   }
   #endif

   #ifdef TLB_DIRECT_MAP
   int i = pgnum % NUM_ENTRY_TLB;
   uint32_t *tag = (uint32_t*)(mp->storage + i*8);
   uint32_t *frmnum = (uint32_t*)(mp->storage + i*8 + 4);
   int valid = TLB_TAG_VALID(*tag);
   int pid_TLB = TLB_TAG_PID(*tag);
   int pgnum_TLB = TLB_TAG_PGN(*tag);

   if (pid_TLB == pid && valid && pgnum_TLB == pgnum) {
      *value = *frmnum;
      pthread_mutex_unlock(&tlb_lock);
      return 0;
   }
   #endif

   pthread_mutex_unlock(&tlb_lock);
   return -1;
}

/*
 *  tlb_cache_write write TLB cache device
 *  @mp: memphy struct
 *  @pid: process id
 *  @pgnum: page number
 *  @value: obtained value
 */
int tlb_cache_write(struct memphy_struct *mp, int pid, int pgnum, int value)
{
   /* TODO: the identify info is mapped to 
    *      cache line by employing:
    *      direct mapped, associated mapping etc.
    */
   // int NUM_ENTRY_TLB = mp->maxsz/10;
   pthread_mutex_lock(&tlb_lock);

   #ifdef TLB_FULLY_ASSOCIATE
   int all_used = 1;
   for (int i = 0; i < NUM_ENTRY_TLB; i++) {
      uint32_t *tag = (uint32_t *)(mp->storage + i * 8);
      uint32_t *frmnum = (uint32_t *)(mp->storage + i * 8 + 4);
      int valid = TLB_TAG_VALID(*tag);

      if (!valid) {
         SETBIT(*tag, TLB_TAG_VALID_MASK);
         SETBIT(*tag, TLB_TAG_USED_MASK);
         SETVAL(*tag, pid, TLB_TAG_PID_MASK, TLB_TAG_PID_LOBIT);
         SETVAL(*tag, pgnum, TLB_TAG_PGN_MASK, TLB_TAG_PGN_LOBIT);
         *frmnum = value;
         all_used = 0;
         break;
      }
   }
   // check LRU
   if (all_used) {
      int lru_index = 0;
      uint32_t lru_tag = UINT32_MAX;
      for (int i = 0; i < NUM_ENTRY_TLB; i++) {
         uint32_t *tag = (uint32_t *)(mp->storage + i * 8);
         uint32_t *frmnum = (uint32_t *)(mp->storage + i * 8 + 4);
         int used = TLB_TAG_USED(*tag);
         if (!used) {
               SETBIT(*tag, TLB_TAG_VALID_MASK);
               SETBIT(*tag, TLB_TAG_USED_MASK);
               SETVAL(*tag, pid, TLB_TAG_PID_MASK, TLB_TAG_PID_LOBIT);
               SETVAL(*tag, pgnum, TLB_TAG_PGN_MASK, TLB_TAG_PGN_LOBIT);
               *frmnum = value;
               break;
         }
         // Find least recently used entry
         if (*tag < lru_tag) {
               lru_index = i;
               lru_tag = *tag;
         }
      }
      // Replace the least recently used entry
      uint32_t *lru_tag_ptr = (uint32_t *)(mp->storage + lru_index * 8);
      uint32_t *lru_frmnum_ptr = (uint32_t *)(mp->storage + lru_index * 8 + 4);

      SETBIT(*lru_tag_ptr, TLB_TAG_VALID_MASK);
      SETBIT(*lru_tag_ptr, TLB_TAG_USED_MASK);
      SETVAL(*lru_tag_ptr, pid, TLB_TAG_PID_MASK, TLB_TAG_PID_LOBIT);
      SETVAL(*lru_tag_ptr, pgnum, TLB_TAG_PGN_MASK, TLB_TAG_PGN_LOBIT);
      *lru_frmnum_ptr = value;
   }
   #endif

   #ifdef TLB_DIRECT_MAP
   int i = pgnum % NUM_ENTRY_TLB;
   uint32_t *tag = (uint32_t*)(mp->storage + i*8);
   uint32_t *frmnum = (uint32_t*)(mp->storage + i*8 + 4);

   SETBIT(*tag, TLB_TAG_VALID_MASK);
   SETVAL(*tag, pid, TLB_TAG_PID_MASK, TLB_TAG_PID_LOBIT);
   SETVAL(*tag, pgnum, TLB_TAG_PGN_MASK, TLB_TAG_PGN_LOBIT);
   *frmnum = value;
   #endif

   pthread_mutex_unlock(&tlb_lock);
   return 0;
}

/*
 *  TLBMEMPHY_read natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int TLBMEMPHY_read(struct memphy_struct * mp, int addr, BYTE *value)
{
   if (mp == NULL)
      return -1;

   pthread_mutex_lock(&tlb_lock);
   /* TLB cached is random access by native */
   *value = mp->storage[addr];
   pthread_mutex_unlock(&tlb_lock);

   return 0;
}


/*
 *  TLBMEMPHY_write natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int TLBMEMPHY_write(struct memphy_struct * mp, int addr, BYTE data)
{
   if (mp == NULL)
      return -1;

   pthread_mutex_lock(&tlb_lock);
   /* TLB cached is random access by native */
   mp->storage[addr] = data;
   pthread_mutex_unlock(&tlb_lock);

   return 0;
}

/*
 *  TLBMEMPHY_format natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 */


int TLBMEMPHY_dump(struct memphy_struct * mp)
{
   /*TODO dump memphy contnt mp->storage 
    *     for tracing the memory content
    */
   
   printf("START_TLB_dump\n");

   #ifdef TLB_FULLY_ASSOCIATE
   for (int i = 0; i < NUM_ENTRY_TLB; i++) {
      uint32_t *tag = (uint32_t*)(mp->storage + i*8);
      uint32_t *frmnum = (uint32_t*)(mp->storage + i*8 + 4);
      int valid = TLB_TAG_VALID(*tag);
      int used = TLB_TAG_USED(*tag);
      int pid = TLB_TAG_PID(*tag);
      int pgnum =  TLB_TAG_PGN(*tag);

      printf("%d %d %08d %08d %08d\n", valid, used, pid, pgnum, *frmnum);
   }
   #endif
   
   #ifdef TLB_DIRECT_MAP
   for (int i = 0; i < NUM_ENTRY_TLB; i++) {
      uint32_t *tag = (uint32_t*)(mp->storage + i*8);
      uint32_t *frmnum = (uint32_t*)(mp->storage + i*8 + 4);
      int valid = TLB_TAG_VALID(*tag);
      int pid = TLB_TAG_PID(*tag);
      int pgnum =  TLB_TAG_PGN(*tag);

      printf("%d %08d %08d %08d\n", valid, pid, pgnum, *frmnum);
   }
   #endif
   printf("END_TLB_dump\n");
   return 0;
}
/*
 *  Init TLBMEMPHY struct
 */
int init_tlbmemphy(struct memphy_struct *mp, int max_size)
{
   mp->storage = (BYTE *)malloc(max_size * sizeof(BYTE));
   if (mp->storage == NULL) {
      // Handle memory allocation failure
      return -1;
   }
   mp->maxsz = max_size;
   mp->rdmflg = 1;
   NUM_ENTRY_TLB = (8 < max_size / 8) ? 8 : (max_size / 8);
   // Initialize TLB cache to 0
   memset(mp->storage, 0, max_size * sizeof(BYTE));
   // Initialize TLB lock
   if (pthread_mutex_init(&tlb_lock, NULL) != 0) {
      // Handle mutex initialization failure
      free(mp->storage);
      return -1;
   }
   return 0;
}

//#endif
