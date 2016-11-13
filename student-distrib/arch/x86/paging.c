#include <arch/x86/paging.h>

static pt_entry vmem_pt[NUM_PT_ENTRIES] __attribute__((aligned (FOUR_KB_ALIGNED)));

void initialize_paging_structs(pd_entry *local_pd) {
    /* Set up remaining Page Table Entries to be empty (8 MB to 4 GigaBytes) */
    {
        int i;
        for(i = 0; i < NUM_PD_ENTRIES; i++) { // 2 represents Page Directory's 3rd entry corresponding to 8 MB in physical memory
            pd_entry blank;
            blank.present = 0;
            local_pd[i] = blank;
        }
    }

    /* Set up 0th Page Table Entry (0 MB to 4 MB) */
    {
        pd_entry video_mem_entry; /* An entry representing first 4 MB of physical memory */
        int i;

        video_mem_entry.physical_addr_31_to_12 = (uint32_t) &vmem_pt[0] >> ADDRESS_SHIFT;
        video_mem_entry.global_ignored  = 0;
        video_mem_entry.page_size       = 0;
        video_mem_entry.dirty_ignored   = 0;
        video_mem_entry.accessed        = 0;
        video_mem_entry.cache_disabled  = 0;
        video_mem_entry.write_through   = 0;
        video_mem_entry.user_accessible = 0;
        video_mem_entry.read_write      = 1;
        video_mem_entry.present         = 1;

        local_pd[0] = video_mem_entry;

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
            vmem_pt[i] = my_entry;
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
        
        local_pd[1] = kernel_page_entry;
    }
}

void setup_process_paging(pd_entry *local_pd, void *process_addr) {
    initialize_paging_structs(local_pd);
    /* Set up Process Page Table Entry (128MB to 132MB -> process_addr) */
    {
        pd_entry process_page_entry;

        // Clear bottom 12 bits, shift over 12 bits
        process_page_entry.physical_addr_31_to_12 = ((uint32_t) process_addr & ~(FOUR_MB_ALIGNED - 1)) >> ADDRESS_SHIFT;
        process_page_entry.global_ignored  = 0;
        process_page_entry.page_size       = 1;
        process_page_entry.dirty_ignored   = 0;
        process_page_entry.accessed        = 0;
        process_page_entry.cache_disabled  = 0;
        process_page_entry.write_through   = 0;
        process_page_entry.user_accessible = 1;
        process_page_entry.read_write      = 1;
        process_page_entry.present         = 1;

        local_pd[32] = process_page_entry;
    }
}