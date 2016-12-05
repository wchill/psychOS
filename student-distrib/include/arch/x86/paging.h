#ifndef _X86_PAGING_H
#define _X86_PAGING_H

#include <types.h>

/* Paging References:
		MP 3.1 Documentation - page 6
		http://wiki.osdev.org/Paging
		Lecture 14 Slides
*/

/* Constants for magic numbers */
#define NUM_PD_ENTRIES    1024     /* max number of entries in Page Directory                      */
#define NUM_PT_ENTRIES    1024     /* max number of entries in Page Table                          */
#define FOUR_KB_ALIGNED   4096     /* bytes in 4 kilobytes that we align structures to             */
#define FOUR_MB_ALIGNED   4194304  /* bytes in 4 megabytes */
#define ADDRESS_SHIFT       12     /* Number of bits to shift (right) offset for 4 KB aligned page */
#define NUM_RESERVED_BITS    3     /* number of bits reserved in PD 4-byte entry                   */
#define NUM_BITS_ADDR       20     /* number of bits to address Page Table or Page. 
                                      However, we only use the top 10 bits if it's a 4MB Page      */

#define PAGING_STRUCT_ADDR (FOUR_MB_ALIGNED * 31)  /* We store paging structs at 124 MB            */
#define PROCESS_STRUCT_SIZE (FOUR_KB_ALIGNED * 4)  /* Our Process structs are 16 KB                */

// Taken from lib.c
#define VIDEO_PHYSICAL_ADDR 0xB8000              /* Physical address of video memory. We think video memory is 4 kb */

/* Page Directory (PD) Entry */
typedef struct __attribute__((packed, aligned(4))) pd_entry { // 4 = align struct on 4-byte boundaries
	union {
		uint32_t val;
		struct {
			uint32_t present                : 1;
			uint32_t read_write             : 1;
			uint32_t user_accessible        : 1;
			uint32_t write_through          : 1;
			uint32_t cache_disabled         : 1;
			uint32_t accessed               : 1;
			uint32_t dirty_ignored          : 1;
			uint32_t page_size              : 1;
			uint32_t global_ignored         : 1;
			uint32_t reserved               : NUM_RESERVED_BITS;
			uint32_t physical_addr_31_to_12 : NUM_BITS_ADDR;
		} __attribute__((packed)) ;
	};
} pd_entry;

/* Page Table (PT) Entry */
typedef struct __attribute__((packed, aligned(4))) pt_entry {
	union {
		uint32_t val;
		struct {
			uint32_t present                : 1;
			uint32_t read_write             : 1;
			uint32_t user_accessible        : 1;
			uint32_t write_through          : 1;
			uint32_t cache_disabled         : 1;
			uint32_t accessed               : 1;
			uint32_t dirty                  : 1;
			uint32_t page_size_ignored      : 1;
			uint32_t global                 : 1;
			uint32_t reserved               : NUM_RESERVED_BITS;
			uint32_t physical_addr_31_to_12 : NUM_BITS_ADDR;
		} __attribute__((packed)) ;
	};
} pt_entry;

extern void enable_paging(pd_entry *table_ptr);
void flush_tlb();
void initialize_paging_structs(pd_entry *local_pd_ptr, pt_entry *local_pt_ptr, void *vmem_addr);
pd_entry *setup_process_paging(void *process_addr, uint32_t slot_num, void *vmem_addr);
void set_process_vmem_page(uint32_t slot_num, void *vmem_addr);
void *get_process_vmem_page(uint32_t process_slot);

#endif
