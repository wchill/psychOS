/* kernel.c - the C part of the kernel
 * vim:ts=4 noexpandtab
 */

#include "multiboot.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "debug.h"
#include "keyboard.h"
#include "interrupt.h"
#include "syscall.h"
#include "rtc.h"

/* Macros. */
/* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags,bit)   ((flags) & (1 << (bit)))

// Taken from lib.c
#define VIDEO 0xB8000

typedef struct pd_entry {
	union {
		uint32_t val;
		struct {
			uint32_t page_table_addr_31_to_12 : 20;
			uint32_t reserved : 3;
			uint32_t global_ignored : 1;
			uint32_t page_size : 1;
			uint32_t dirty_ignored : 1;
			uint32_t accessed : 1;
			uint32_t cache_disabled : 1;
			uint32_t write_through : 1;
			uint32_t user_accessible : 1;
			uint32_t read_write : 1;
			uint32_t present : 1;
		} __attribute__((packed)) ;
	};
} pd_entry;

typedef struct __attribute__((packed)) pt_entry {
	union {
		uint32_t val;
		struct {
			uint32_t physical_addr_31_to_12 : 20;
			uint32_t reserved : 3;
			uint32_t global : 1;
			uint32_t page_size_ignored : 1;
			uint32_t dirty : 1;
			uint32_t accessed : 1;
			uint32_t cache_disabled : 1;
			uint32_t write_through : 1;
			uint32_t user_accessible : 1;
			uint32_t read_write : 1;
			uint32_t present : 1;
		} __attribute__((packed)) ;
	};
} pt_entry;

static pd_entry paging_directory[1024];
static pt_entry video_mem_page_table[1024];

extern void enable_paging(pd_entry *table_ptr);

void initialize_paging() {
	{
		pd_entry video_mem_entry;
		int i;

		video_mem_entry.page_table_addr_31_to_12 = (uint32_t) &video_mem_page_table[0];
		video_mem_entry.global_ignored = 0;
		video_mem_entry.page_size = 0;
		video_mem_entry.dirty_ignored = 0;
		video_mem_entry.accessed = 0;
		video_mem_entry.cache_disabled = 0;
		video_mem_entry.write_through = 0;
		video_mem_entry.user_accessible = 0;
		video_mem_entry.read_write = 1;
		video_mem_entry.present = 1;
		paging_directory[0] = video_mem_entry;

		for(i = 0; i < 1024; i++) {
			uint32_t addr = 0x1000 * i;
			pt_entry my_entry;

			if(addr != VIDEO) {
				my_entry.present = 0;
			} else {
				my_entry.physical_addr_31_to_12 = (uint32_t) VIDEO >> 12;
				my_entry.global = 1;
				my_entry.page_size_ignored = 0;
				my_entry.dirty = 0;
				my_entry.accessed = 0;
				my_entry.cache_disabled = 1;
				my_entry.write_through = 0;
				my_entry.user_accessible = 0;
				my_entry.read_write = 1;
				my_entry.present = 1;
			}
			video_mem_page_table[i] = my_entry;
		}
	}

	{
		pd_entry kernel_page_entry;

		kernel_page_entry.page_table_addr_31_to_12 = 4194304 >> 12; // address 4MB
		kernel_page_entry.global_ignored = 0;
		kernel_page_entry.page_size = 1;
		kernel_page_entry.dirty_ignored = 0;
		kernel_page_entry.accessed = 0;
		kernel_page_entry.cache_disabled = 0;
		kernel_page_entry.write_through = 0;
		kernel_page_entry.user_accessible = 0;
		kernel_page_entry.read_write = 1;
		kernel_page_entry.present = 1;
		paging_directory[1] = kernel_page_entry;
	}
	
	{
		int i;
		for(i = 2; i < 1024; i++) {
			pd_entry blank;
			blank.present = 0;
			paging_directory[i] = blank;
		}
	}

	enable_paging(&paging_directory[0]);
}

/* Check if MAGIC is valid and print the Multiboot information structure
   pointed by ADDR. */
void
entry (unsigned long magic, unsigned long addr)
{
	multiboot_info_t *mbi;

	/* Clear the screen. */
	clear();

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
	}

	printf("Initializing the PIC\n");

	/* Init the PIC */
	i8259_init();

	// Uncomment below to cause divide-by-zero exception
	//asm volatile("movl $0, %eax; divl %eax;");

	/* Initialize devices, memory, filesystem, enable device interrupts on the
	 * PIC, any other initialization stuff... */

	// Shouldn't need to do any keyboard initialization other than enabling IRQ, but just in case:
	// http://wiki.osdev.org/%228042%22_PS/2_Controller#Initialising_the_PS.2F2_Controller
	{
		install_interrupt_handler(IRQ_INT_NUM(KEYBOARD_IRQ), keyboard_handler_wrapper, KERNEL_CS, PRIVILEGE_KERNEL);
		enable_irq(KEYBOARD_IRQ);
	}
	
	// Need to initialize RTC before enabling IRQ
	// See https://courses.engr.illinois.edu/ece391/secure/references/mc146818.pdf
	// http://wiki.osdev.org/RTC#Programming_the_RTC
	// https://piazza.com/class/iqsg6pdvods1rw?cid=596
	{
		char prev;

		outb(RTC_DISABLE_NMI | RTC_REG_B, RTC_STATUS_PORT);     	// select register B, and disable NMI
		prev = inb(RTC_DATA_PORT);									// read the current value of register B
		outb(RTC_DISABLE_NMI | RTC_REG_B, RTC_STATUS_PORT);			// set the index again (a read will reset the index to register D)
		outb(prev | RTC_ENABLE_INTERRUPTS, RTC_DATA_PORT);	// write the previous value ORed with 0x40. This turns on bit 6 of register B

		// 2Hz interrupt rate
		uint8_t rate = 15;										// 32768 >> (rate - 1) = 2 gives us 15
		outb(RTC_DISABLE_NMI | RTC_REG_A, RTC_STATUS_PORT);
		prev = inb(RTC_DATA_PORT);								// get initial value of register A
		outb(RTC_DISABLE_NMI | RTC_REG_A, RTC_STATUS_PORT);		// reset index to A
		outb((prev & 0xF0) | rate, RTC_DATA_PORT);				//write only our rate to A. Note, rate is the bottom 4 bits, so keep the top 4 bits.

		install_interrupt_handler(IRQ_INT_NUM(RTC_IRQ), rtc_handler_wrapper, KERNEL_CS, PRIVILEGE_KERNEL);
		enable_irq(RTC_IRQ);
	}

	printf("Initializing Paging\n");
	initialize_paging();

	/* Enable interrupts */
	/* Do not enable the following until after you have set up your
	 * IDT correctly otherwise QEMU will triple fault and simple close
	 * without showing you any output */
	printf("Enabling Interrupts\n");
	sti();

	/* Execute the first program (`shell') ... */

	/* Spin (nicely, so we don't chew up cycles) */
	for(;;) {
    	asm("hlt");
 	}
}

