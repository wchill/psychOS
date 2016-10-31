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
#define ADDRESS_SHIFT       12     /* Number of bits to shift (right) offset for 4 KB aligned page */
#define NUM_RESERVED_BITS    3     /* number of bits reserved in PD 4-byte entry                   */
#define NUM_BITS_ADDR       20     /* number of bits to address Page Table or Page. 
                                      However, we only use the top 10 bits if it's a 4MB Page      */

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
typedef struct __attribute__((packed)) pt_entry {
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
void initialize_paging();

#endif
