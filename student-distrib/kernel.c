/* kernel.c - the C part of the kernel
 * vim:ts=4 noexpandtab
 */

#include "multiboot.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "debug.h"
#include "ece391_fs.h"
#include "terminal.h"
#include "interrupt.h"
#include "syscall.h"
#include "rtc.h"
#include "tests.h" // added for 3.2

/* Macros. */
/* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags,bit)   ((flags) & (1 << (bit)))

// Taken from lib.c
#define VIDEO 0xB8000              /* Physical address of video memory. We think video memory is 4 kb */

/* Constants for magic numbers */
#define NUM_PD_ENTRIES    1024     /* max number of entries in Page Directory                      */
#define NUM_PT_ENTRIES    1024     /* max number of entries in Page Table                          */
#define FOUR_KB_ALIGNED   4096     /* bytes in 4 kilobytes that we align structures to             */
#define ADDRESS_SHIFT       12     /* Number of bits to shift (right) offset for 4 KB aligned page */
#define NUM_RESERVED_BITS    3     /* number of bits reserved in PD 4-byte entry                   */
#define NUM_BITS_ADDR       20     /* number of bits to address Page Table or Page. 
                                      However, we only use the top 10 bits if it's a 4MB Page      */

/* Constants for 3.2 */
#define BUFFER_4K 4096
#define KEYBOARD_SIZE 128

/* Paging References:
		MP 3.1 Documentation - page 6
		http://wiki.osdev.org/Paging
		Lecture 14 Slides
*/

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

/* We store Page Directory and Page Tables in Kernel. Proper alignment is necessary (mp3 doc. p.6) */
static pd_entry paging_directory[NUM_PD_ENTRIES] __attribute__((aligned (FOUR_KB_ALIGNED)));     // PD = Page Directory
static pt_entry video_mem_page_table[NUM_PT_ENTRIES] __attribute__((aligned (FOUR_KB_ALIGNED))); // PT = Page Table

extern void enable_paging(pd_entry *table_ptr);

/*
 * initalize_paging
 *   DESCRIPTION:  Initializes paging by filling PD and PT kernel arrays
 *   INPUTS:       none
 *   OUTPUTS:      none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: fills Page Directory and Page Table with appropriate values
 */   
void initialize_paging() {

	/* Set up 0th Page Table Entry (0 MB to 4 MB) */
	{
		pd_entry video_mem_entry; /* An entry representing first 4 MB of physical memory */
		int i;

		video_mem_entry.physical_addr_31_to_12 = (uint32_t) &video_mem_page_table[0] >> ADDRESS_SHIFT;
		video_mem_entry.global_ignored  = 0;
		video_mem_entry.page_size       = 0;
		video_mem_entry.dirty_ignored   = 0;
		video_mem_entry.accessed        = 0;
		video_mem_entry.cache_disabled  = 0;
		video_mem_entry.write_through   = 0;
		video_mem_entry.user_accessible = 0;
		video_mem_entry.read_write      = 1;
		video_mem_entry.present         = 1;

		paging_directory[0] = video_mem_entry;

		for(i = 0; i < NUM_PT_ENTRIES; i++) {
			uint32_t addr = FOUR_KB_ALIGNED * i;
			pt_entry my_entry;

			if(addr != VIDEO) {
				my_entry.present = 0;
			} else {
				my_entry.physical_addr_31_to_12 = (uint32_t) addr >> ADDRESS_SHIFT;
				my_entry.global            = 0; // Rodney: I changed this to 0
				my_entry.page_size_ignored = 0;
				my_entry.dirty             = 0;
				my_entry.accessed          = 0;
				my_entry.cache_disabled    = 0; // Rodney: I changed this to 0
				my_entry.write_through     = 0;
				my_entry.user_accessible   = 0;
				my_entry.read_write        = 1;
				my_entry.present           = 1;
			}
			video_mem_page_table[i] = my_entry;
		}
	}

	/* Set up 1st Page Table Entry (4 MB to 8 MB) */
	{
		pd_entry kernel_page_entry;

		kernel_page_entry.physical_addr_31_to_12 = 1024; // 1024 represents address 4 MegaBytes. That's cuz we use ALL 32 bits for physical address.
		kernel_page_entry.global_ignored  = 1;
		kernel_page_entry.page_size       = 1;
		kernel_page_entry.dirty_ignored   = 0;
		kernel_page_entry.accessed        = 0;
		kernel_page_entry.cache_disabled  = 0;
		kernel_page_entry.write_through   = 0;
		kernel_page_entry.user_accessible = 0;
		kernel_page_entry.read_write      = 1;
		kernel_page_entry.present         = 1;

		//kernel_page_entry.val = 0x400183; // Rodney: manually overwrites to value I want. The above code should set it to 0x400183 (the 4 in 0x400183 is most important)

		paging_directory[1] = kernel_page_entry;
	}
	
	/* Set up remaining Page Table Entries to be empty (8 MB to 4 GigaBytes) */
	{
		int i;
		for(i = 2; i < NUM_PD_ENTRIES; i++) { // 2 represents Page Directory's 3rd entry corresponding to 8 MB in physical memory
			pd_entry blank;
			blank.present = 0;
			paging_directory[i] = blank;
		}
	}

	enable_paging(&paging_directory[0]); // defined in paging.S
}

/* Check if MAGIC is valid and print the Multiboot information structure
   pointed by ADDR. */
void
entry (unsigned long magic, unsigned long addr)
{
	multiboot_info_t *mbi;

	/* Clear the screen. */
	clear_terminal();

	/* Am I booted by a Multiboot-compliant boot loader? */
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
	{
		printf ("Invalid magic number: 0x%#x\n", (unsigned) magic);
		return;
	}

	/* Set MBI to the address of the Multiboot information structure. */
	mbi = (multiboot_info_t *) addr;

	/* Print out the flags. */
	printf ("flags = 0x%#x\n", (unsigned) mbi->flags);

	/* Are mem_* valid? */
	if (CHECK_FLAG (mbi->flags, 0))
		printf ("mem_lower = %uKB, mem_upper = %uKB\n",
				(unsigned) mbi->mem_lower, (unsigned) mbi->mem_upper);

	/* Is boot_device valid? */
	if (CHECK_FLAG (mbi->flags, 1))
		printf ("boot_device = 0x%#x\n", (unsigned) mbi->boot_device);

	/* Is the command line passed? */
	if (CHECK_FLAG (mbi->flags, 2))
		printf ("cmdline = %s\n", (char *) mbi->cmdline);

	if (CHECK_FLAG (mbi->flags, 3)) {
		int mod_count = 0;
		int i;
		module_t* mod = (module_t*)mbi->mods_addr;
		while(mod_count < mbi->mods_count) {
			printf("Module %d loaded at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_start);
			printf("Module %d ends at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_end);
			printf("First few bytes of module:\n");
			for(i = 0; i<16; i++) {
				printf("0x%x ", *((char*)(mod->mod_start+i)));
			}
			printf("\n");
			mod_count++;
			mod++;
		}
	}
	/* Bits 4 and 5 are mutually exclusive! */
	if (CHECK_FLAG (mbi->flags, 4) && CHECK_FLAG (mbi->flags, 5))
	{
		printf ("Both bits 4 and 5 are set.\n");
		return;
	}

	/* Is the section header table of ELF valid? */
	if (CHECK_FLAG (mbi->flags, 5))
	{
		elf_section_header_table_t *elf_sec = &(mbi->elf_sec);

		printf ("elf_sec: num = %u, size = 0x%#x,"
				" addr = 0x%#x, shndx = 0x%#x\n",
				(unsigned) elf_sec->num, (unsigned) elf_sec->size,
				(unsigned) elf_sec->addr, (unsigned) elf_sec->shndx);
	}

	/* Are mmap_* valid? */
	if (CHECK_FLAG (mbi->flags, 6))
	{
		memory_map_t *mmap;

		printf ("mmap_addr = 0x%#x, mmap_length = 0x%x\n",
				(unsigned) mbi->mmap_addr, (unsigned) mbi->mmap_length);
		for (mmap = (memory_map_t *) mbi->mmap_addr;
				(unsigned long) mmap < mbi->mmap_addr + mbi->mmap_length;
				mmap = (memory_map_t *) ((unsigned long) mmap
					+ mmap->size + sizeof (mmap->size)))
			printf (" size = 0x%x,     base_addr = 0x%#x%#x\n"
					"     type = 0x%x,  length    = 0x%#x%#x\n",
					(unsigned) mmap->size,
					(unsigned) mmap->base_addr_high,
					(unsigned) mmap->base_addr_low,
					(unsigned) mmap->type,
					(unsigned) mmap->length_high,
					(unsigned) mmap->length_low);
	}

	/* Construct an LDT entry in the GDT */
	{
		seg_desc_t the_ldt_desc;
		the_ldt_desc.granularity    = 0;
		the_ldt_desc.opsize         = 1;
		the_ldt_desc.reserved       = 0;
		the_ldt_desc.avail          = 0;
		the_ldt_desc.present        = 1;
		the_ldt_desc.dpl            = 0x0;
		the_ldt_desc.sys            = 0;
		the_ldt_desc.type           = 0x2;

		SET_LDT_PARAMS(the_ldt_desc, &ldt, ldt_size);
		ldt_desc_ptr = the_ldt_desc;
		lldt(KERNEL_LDT);
	}

	/* Construct a TSS entry in the GDT */
	{
		seg_desc_t the_tss_desc;
		the_tss_desc.granularity    = 0;
		the_tss_desc.opsize         = 0;
		the_tss_desc.reserved       = 0;
		the_tss_desc.avail          = 0;
		the_tss_desc.seg_lim_19_16  = TSS_SIZE & 0x000F0000;
		the_tss_desc.present        = 1;
		the_tss_desc.dpl            = 0x0;
		the_tss_desc.sys            = 0;
		the_tss_desc.type           = 0x9;
		the_tss_desc.seg_lim_15_00  = TSS_SIZE & 0x0000FFFF;

		SET_TSS_PARAMS(the_tss_desc, &tss, tss_size);

		tss_desc_ptr = the_tss_desc;

		tss.ldt_segment_selector = KERNEL_LDT;
		tss.ss0 = KERNEL_DS;
		tss.esp0 = 0x800000;
		ltr(KERNEL_TSS);
	}

	/* Set up the IDT */
	{
		void (*handlers[NUM_RESERVED_VEC]) (void);
		int i;

		for(i = 0; i < NUM_VEC; i++) {
			install_interrupt_handler(i, null_interrupt_handler, KERNEL_CS, PRIVILEGE_KERNEL);
		}

		lidt(idt_desc_ptr);
		handlers[0] = interrupt_handler_0;
		handlers[1] = interrupt_handler_1;
		handlers[2] = interrupt_handler_2;
		handlers[3] = interrupt_handler_3;
		handlers[4] = interrupt_handler_4;
		handlers[5] = interrupt_handler_5;
		handlers[6] = interrupt_handler_6;
		handlers[7] = interrupt_handler_7;
		handlers[8] = interrupt_handler_8;
		handlers[9] = interrupt_handler_9;
		handlers[10] = interrupt_handler_10;
		handlers[11] = interrupt_handler_11;
		handlers[12] = interrupt_handler_12;
		handlers[13] = interrupt_handler_13;
		handlers[14] = interrupt_handler_14;
		handlers[15] = interrupt_handler_15;
		handlers[16] = interrupt_handler_16;
		handlers[17] = interrupt_handler_17;
		handlers[18] = interrupt_handler_18;
		handlers[19] = interrupt_handler_19;
		handlers[20] = interrupt_handler_20;
		handlers[21] = interrupt_handler_21;
		handlers[22] = interrupt_handler_22;
		handlers[23] = interrupt_handler_23;
		handlers[24] = interrupt_handler_24;
		handlers[25] = interrupt_handler_25;
		handlers[26] = interrupt_handler_26;
		handlers[27] = interrupt_handler_27;
		handlers[28] = interrupt_handler_28;
		handlers[29] = interrupt_handler_29;
		handlers[30] = interrupt_handler_30;
		handlers[31] = interrupt_handler_31;

		for(i = 0; i < NUM_RESERVED_VEC; i++) {
			install_interrupt_handler(i, handlers[i], KERNEL_CS, PRIVILEGE_KERNEL);
		}

		// Handle syscalls
		install_interrupt_handler(SYSCALL_INT, syscall_handler_wrapper, KERNEL_CS, PRIVILEGE_USER);

        // Install device handlers
        install_interrupt_handler(IRQ_INT_NUM(KEYBOARD_IRQ), keyboard_handler_wrapper, KERNEL_CS, PRIVILEGE_KERNEL);
        install_interrupt_handler(IRQ_INT_NUM(RTC_IRQ), rtc_handler_wrapper, KERNEL_CS, PRIVILEGE_KERNEL);
	}

	printf("Initializing the PIC\n");

	/* Init the PIC */
	i8259_init();

	// Uncomment below to cause divide-by-zero exception
	//asm volatile("movl $0, %eax; divl %eax;");

	/* Initialize devices, memory, filesystem, enable device interrupts on the
	 * PIC, any other initialization stuff... */
    {
        // Initialize file system
        // Seems like we don't need to worry about mapping pages for the FS?
        // The FS is loaded into memory somewhere above 0x400000 but less than 0x800000,
        // so it's still within the kernel's 4MB page
        printf("Initializing ECE391 File System\n");
        module_t* mod = (module_t*) mbi->mods_addr;
        ece391_fs_init((void*) mod->mod_start);
    }
	
	printf("Initializing Paging\n");
	initialize_paging();

	/* Enable interrupts */
	/* Do not enable the following until after you have set up your
	 * IDT correctly otherwise QEMU will triple fault and simple close
	 * without showing you any output */
	printf("Enabling Interrupts\n");
	sti();

    terminal_open("stdin");

    //rtc_read(); // tests "rtc_read" since code doesn't infinitely loop here, that means the function returned.

	/* Execute the first program (`shell') ... */
	
    // {
    //     uint8_t input[KEYBOARD_SIZE];
    //     int i;
    //     for(i = 0; i < 63; i++) { // 63 plus 1 is the number of dentrys we read
    //         dentry_t dentry;
    //         int32_t res = read_dentry_by_index(i, &dentry);
    //         if(res >= 0) {
    //             terminal_write(0, dentry.file_name, 32); // 32 is number of bytes we're writing to terminal
    //             char c = '\n';
    //             terminal_write(0, &c, 1);
    //         }
    //     }
    //     {
    //         dentry_t dentry;
    //         int32_t res = read_dentry_by_name("verylargetextwithverylongname.tx", &dentry);
    //         int32_t num_read = 0;
    //         if(res >= 0) {
    //             res = 0;
    //             uint8_t buf[BUFFER_4K];
    //             do {
    //                 res = read_data(dentry.inode_num, num_read, buf, BUFFER_4K);
    //                 terminal_write(0, buf, res);
    //                 num_read += res;
    //             } while(res > 0);
    //         }
    //     }
    //     terminal_read(0, input, KEYBOARD_SIZE);
    //     {
    //         dentry_t dentry;
    //         int32_t res = read_dentry_by_name("shell", &dentry);
    //         int32_t num_read = 0;
    //         if(res >= 0) {
    //             res = 0;
    //             uint8_t buf[BUFFER_4K];
    //             do {
    //                 res = read_data(dentry.inode_num, num_read, buf, BUFFER_4K);
    //                 terminal_write(0, buf, res);
    //                 num_read += res;
    //             } while(res > 0);
    //         }
    //     }
    // }
    
	/* Spin (nicely, so we don't chew up cycles) */
    uint8_t input[KEYBOARD_SIZE];
	for(;;) {
    	asm("hlt");
        terminal_read(0, input, KEYBOARD_SIZE);
 	}
}
