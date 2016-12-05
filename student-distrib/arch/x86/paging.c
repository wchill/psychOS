#include <arch/x86/paging.h>
#include <arch/x86/task.h>

static pt_entry vmem_pt[NUM_PT_ENTRIES] __attribute__((aligned (FOUR_KB_ALIGNED)));

static pd_entry *get_process_pd(uint32_t slot_num) {
    return (pd_entry*) (PAGING_STRUCT_ADDR + PROCESS_STRUCT_SIZE * slot_num);
}

static pt_entry *get_process_pt(uint32_t slot_num) {
    return (pt_entry*) (PAGING_STRUCT_ADDR + PROCESS_STRUCT_SIZE * slot_num + FOUR_KB_ALIGNED);
}

/**
 * initialize_paging_structs
 * Initializes paging structures by doing:
 *    1) Set up 0-4 MB memory, including video memory
 *    2) set up 4-8 MB Kernel page (mapped directly from video memory)
 *    3) Set up pages for storing page directories/page structs (at 124-128 MB)
 *    4) set up program pages for the processes (at 128-132 MB)
 *    5) set up process VMEM page (at 132 MB). All processes have the same VMEM page.
 *    everything else is set to blank
 * 
 * @param local_pd_ptr  pointer to a Page Directory entry. We want it to be the 0th PD entry.
 */
void initialize_paging_structs(pd_entry *local_pd_ptr, pt_entry *local_pt_ptr, void *vmem_addr) {
    /* Make all Page Directory (PD) entries blank for now */
    {
        int i;
        for(i = 0; i < NUM_PD_ENTRIES; i++) {
            pd_entry blank;
            blank.present = 0;
            local_pd_ptr[i] = blank;
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

        local_pd_ptr[0] = video_mem_entry;

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
        
        local_pd_ptr[1] = kernel_page_entry;
    }

    /* Set up pages for storing page directories/page structs */
    {
        // Allocate 4MB at address 124MB
        pd_entry kernel_page_entry;

        kernel_page_entry.physical_addr_31_to_12 = PAGING_STRUCT_ADDR >> ADDRESS_SHIFT;
        kernel_page_entry.global_ignored  = 1;
        kernel_page_entry.page_size       = 1;
        kernel_page_entry.dirty_ignored   = 0;
        kernel_page_entry.accessed        = 0;
        kernel_page_entry.cache_disabled  = 0;
        kernel_page_entry.write_through   = 0;
        kernel_page_entry.user_accessible = 0;
        kernel_page_entry.read_write      = 1;
        kernel_page_entry.present         = 1;
        
        local_pd_ptr[31] = kernel_page_entry;
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

            local_pd_ptr[2 + i] = process_page_entry; // 2 represents the offset so that the process pages start at 8 MB
        }
    }

    {
        pd_entry video_mem_entry; /* An entry representing first 4 MB of physical memory */
        int i;

        video_mem_entry.physical_addr_31_to_12 = (uint32_t) local_pt_ptr >> ADDRESS_SHIFT;
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
        local_pd_ptr[33] = video_mem_entry;

        {
            pt_entry my_entry;

            my_entry.physical_addr_31_to_12 = (uint32_t) vmem_addr >> ADDRESS_SHIFT;
            my_entry.global                 = 0;
            my_entry.page_size_ignored      = 0;
            my_entry.dirty                  = 0;
            my_entry.accessed               = 0;
            my_entry.cache_disabled         = 0;
            my_entry.write_through          = 0;
            my_entry.user_accessible        = 1;
            my_entry.read_write             = 1;
            my_entry.present                = 1;

            local_pt_ptr[0] = my_entry;
        }

        for(i = 1; i < NUM_PT_ENTRIES; i++) {
            // We only support up to MAX_PROCESSES, rest of the page table entries should be non present
            pt_entry my_entry;
            my_entry.present = 0;

            local_pt_ptr[i] = my_entry;
        }
    }
}

/**
 * setup_process_paging
 * Sets up paging for a single process
 * 
 * @param process_addr  a pointer to processes address.
 */
pd_entry *setup_process_paging(void *process_addr, uint32_t slot_num, void *vmem_addr) {
    pd_entry *local_pd = get_process_pd(slot_num);
    pt_entry *local_pt = get_process_pt(slot_num);
    initialize_paging_structs(local_pd, local_pt, vmem_addr);
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
    local_pt[0].user_accessible = 1;

    return local_pd;
}

void flush_tlb() {
    asm volatile(
        "mov %%cr3, %%eax\r\n"
        "mov %%eax, %%cr3\r\n"
        :
        :
        : "memory", "eax"
    );
}

/**
 * set_process_vmem_page
 * Updates the process's page tables to point to virtual memory.
 * 
 * @param slot_num  number of the process. Between 0 and (MAX_PROCESSES-1). Used to find process's page tables.
 * @param vmem_addr address of video memory that will be saved by the process's page table.
 */
void set_process_vmem_page(uint32_t slot_num, void *vmem_addr) {
    pt_entry *local_pt = get_process_pt(slot_num);
    local_pt[0].physical_addr_31_to_12 = (uint32_t) vmem_addr >> ADDRESS_SHIFT;
    local_pt[0].present = 1;
    flush_tlb();
}

/**
 * get_process_vmem_page
 * Returns a static virtual memory address (132 MB) which is where our process VMEM (video memory) page is located
 *
 * @param process_slot  The number of the process to return the VMEM page for. Between 0 and (MAX_PROCESSES-1). This number is discarded though.
 */
void *get_process_vmem_page(uint32_t process_slot) {
    // Always 132MB
    return (void*) (33 * FOUR_MB_ALIGNED); // 33 Represents the PD entry that will correspond to 132 MB, which is where the Process VMEM page is located.
}
