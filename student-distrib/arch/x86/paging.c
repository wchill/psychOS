#include <arch/x86/paging.h>

/* We store Page Directory and Page Tables in Kernel. Proper alignment is necessary (mp3 doc. p.6) */
static pd_entry paging_directory[NUM_PD_ENTRIES] __attribute__((aligned (FOUR_KB_ALIGNED)));     // PD = Page Directory
static pt_entry video_mem_page_table[NUM_PT_ENTRIES] __attribute__((aligned (FOUR_KB_ALIGNED))); // PT = Page Table

/*
 * initalize_paging
 * fills Page Directory and Page Table with appropriate values
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

            if(addr != VIDEO_PHYSICAL_ADDR) {
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
