#include <arch/x86/paging.h>
#include <arch/x86/task.h>

static pt_entry vmem_pt[NUM_PT_ENTRIES] __attribute__((aligned (FOUR_KB_ALIGNED)));
static pt_entry process_vmem_pt[NUM_PT_ENTRIES] __attribute__((aligned (FOUR_KB_ALIGNED)));

/**
 * initialize_paging_structs
 * Initializes paging structures by doing:
 *    1) set up video memory
 *    2) set up Kernel page
 *    3) set up program pages for the processes
 *    everything else is set to blank
 * 
 * @param local_pd  pointer to a Page Directory entry. We want it to be the 0th PD entry.
 */
void initialize_paging_structs(pd_entry *local_pd) {
    /* Make all Page Directory (PD) entries blank for now */
    {
        int i;
        for(i = 0; i < NUM_PD_ENTRIES; i++) {
            pd_entry blank;
            blank.present = 0;
            local_pd[i] = blank;
        }
    }

    /* Set up 0th Page Directory Entry (0 MB to 4 MB) */
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

    /* Set up 1st Page Directory Entry (4 MB to 8 MB) */
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

    /* Set up program pages */
    {
        int i;
        for(i = 0; i < MAX_PROCESSES; i++) {
            pd_entry process_page_entry;

            process_page_entry.physical_addr_31_to_12 = 2048 + 1024 * i; // 2048 represents address 8 MegaBytes. 1024 represents 4 MB. we want program 1 at 8 MB, program 2 at 12 MB, etc.
            process_page_entry.global_ignored  = 0;
            process_page_entry.page_size       = 1;
            process_page_entry.dirty_ignored   = 0;
            process_page_entry.accessed        = 0;
            process_page_entry.cache_disabled  = 0;
            process_page_entry.write_through   = 0;
            process_page_entry.user_accessible = 0;
            process_page_entry.read_write      = 1;
            process_page_entry.present         = 1;

            local_pd[2 + i] = process_page_entry; // 2 represents the offset so that the process pages start at 8 MB
        }
    }

    {
        pd_entry video_mem_entry; /* An entry representing first 4 MB of physical memory */
        int i;

        video_mem_entry.physical_addr_31_to_12 = (uint32_t) &process_vmem_pt[0] >> ADDRESS_SHIFT;
        video_mem_entry.global_ignored  = 0;
        video_mem_entry.page_size       = 0;
        video_mem_entry.dirty_ignored   = 0;
        video_mem_entry.accessed        = 0;
        video_mem_entry.cache_disabled  = 0;
        video_mem_entry.write_through   = 0;
        video_mem_entry.user_accessible = 1;
        video_mem_entry.read_write      = 1;
        video_mem_entry.present         = 1;

        // Page directory entry for 132MB - 136MB (132MB = 4MB * 33)
        local_pd[33] = video_mem_entry;

        for(i = 0; i < MAX_PROCESSES; i++) {
            // Initialize page table entry for each process
            // Make it not user accessible until we actually load that process
            pt_entry my_entry;

            my_entry.physical_addr_31_to_12 = (uint32_t) VIDEO_PHYSICAL_ADDR >> ADDRESS_SHIFT;
            my_entry.global                 = 1;
            my_entry.page_size_ignored      = 0;
            my_entry.dirty                  = 0;
            my_entry.accessed               = 0;
            my_entry.cache_disabled         = 0;
            my_entry.write_through          = 0;
            my_entry.user_accessible        = 0;
            my_entry.read_write             = 1;
            my_entry.present                = 1;

            process_vmem_pt[i] = my_entry;
        }

        for(i = MAX_PROCESSES; i < NUM_PT_ENTRIES; i++) {
            // We only support up to MAX_PROCESSES, rest of the page table entries should be non present
            pt_entry my_entry;
            my_entry.present = 0;

            process_vmem_pt[i] = my_entry;
        }
    }
}

/**
 * setup_process_paging
 * Sets up a 4 MB page for a single process
 * 
 * @param local_pd      pointer to a Page Directory entry. We us it as an array of page directory entries.
 * @param process_addr  a pointer to processes address.
 */
void setup_process_paging(pd_entry *local_pd, void *process_addr, uint32_t slot_num) {
    initialize_paging_structs(local_pd);
    /* Set up Process Page Directory Entry (128MB to 132MB -> process_addr) */
    {
        pd_entry process_page_entry;

        process_page_entry.physical_addr_31_to_12 = ((uint32_t) process_addr & ~(FOUR_MB_ALIGNED - 1)) >> ADDRESS_SHIFT; // Rodney: Is bitmask redundant? Won't process_addr already have bottom 22 bits as 0?
        process_page_entry.global_ignored  = 0;
        process_page_entry.page_size       = 1;
        process_page_entry.dirty_ignored   = 0;
        process_page_entry.accessed        = 0;
        process_page_entry.cache_disabled  = 0;
        process_page_entry.write_through   = 0;
        process_page_entry.user_accessible = 1;
        process_page_entry.read_write      = 1;
        process_page_entry.present         = 1;

        local_pd[32] = process_page_entry; // 32 represents the PD entry that will correspond to 128 MB in physical memory (since 32 * 4 = 128)
    }

    // Make process video memory user accessible
    process_vmem_pt[slot_num].user_accessible = 1;
}

void *get_process_vmem_page(uint32_t process_slot) {
    // Formula: 132MB (33 * 4MB) + (process number * 4KB)
    // 1st process's video memory starts at 132MB, 2nd process at 132MB + 4KB, 3rd at 132MB + 8KB, ...
    return (void*) (33 * FOUR_MB_ALIGNED + process_slot * FOUR_KB_ALIGNED);
}