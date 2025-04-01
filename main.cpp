/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <cassert>
#include <iostream>
#include <string.h>
#include <vector>

using namespace std;

// Prototype for test program
typedef void (*program_f)(char *data, int length);

// Number of physical frames
int nframes;

// Pointer to disk for access from handlers
struct disk *disk = nullptr;
vector<bool> frames_used;



// Simple handler for pages == frames
void page_fault_handler_random_eviction(struct page_table *pt, int page)
{
    cout << "page fault on page #" << page << endl;

    // Print the page table contents
    cout << "Before ---------------------------" << endl;
    page_table_print(pt);
    cout << "----------------------------------" << endl;

    // case for chnageing page to W
    int * bits = new int;
    int * framenumber = new int;
    page_table_get_entry(pt, page, framenumber, bits);
    if (*bits != PROT_WRITE && *bits == PROT_READ ) // making a page dirty
    {
        page_table_set_entry(pt, page, *framenumber, PROT_READ | PROT_WRITE);
    } else if (*bits != PROT_READ) { // if the page needs to be alloced in Physcial mem
        // we need at add this to the page table 
        //check if frames are available
        bool already_allocated = false;

        for (int i = 0; i < nframes; i++) {
            if (frames_used[i] == false) {
                // Found an empty frame
                frames_used[i] = true;
                
                disk_read(disk, page, page_table_get_physmem(pt) + (i * PAGE_SIZE));
                page_table_set_entry(pt, page, i, PROT_READ);
                already_allocated = true;
                break;
            }
        }
        if (!already_allocated) {
            // No empty frames, we need to evict a page
            int random_page = rand() % page_table_get_npages(pt);
            int* replaced_page = new int;
            *replaced_page = random_page;

            int* replaced_page_bits = new int;
            int* replaced_frame = new int;
            page_table_get_entry(pt, random_page, replaced_frame, replaced_page_bits);

            if (*replaced_page_bits == PROT_WRITE){
                // Replaced page is dirty, we need to write it to the disk before replacing
                disk_write(disk, *replaced_page, page_table_get_physmem(pt) + (*replaced_frame * PAGE_SIZE));
                disk_read(disk, page, page_table_get_physmem(pt) + (*framenumber * PAGE_SIZE));
                page_table_set_entry(pt, page, *replaced_frame, PROT_READ);
                page_table_set_entry(pt, *replaced_page, *replaced_frame, 0);
            } else{
                // Replaced page is clean, we can just replace it
                disk_read(disk, page, page_table_get_physmem(pt) + (*framenumber * PAGE_SIZE));
                page_table_set_entry(pt, *replaced_page, *replaced_frame, 0);
                page_table_set_entry(pt, page, *framenumber, PROT_READ);
            }
        }
    }



    // Map the page to the same frame number and set to read/write
    // TODO - Disable exit and enable page table update for example
    exit(1);
    // page_table_set_entry(pt, page, page, PROT_READ | PROT_WRITE);

    // Print the page table contents
    cout << "After ----------------------------" << endl;
    page_table_print(pt);
    cout << "----------------------------------" << endl;
}

// TODO - Handler(s) and page eviction algorithms

int main(int argc, char *argv[])
{
    // Check argument count
    if (argc != 5)
    {
        cerr << "Usage: virtmem <npages> <nframes> <rand|fifo|custom> <sort|scan|focus>" << endl;
        exit(1);
    }

    // Parse command line arguments
    int npages = atoi(argv[1]);
    nframes = atoi(argv[2]);
    const char *algorithm = argv[3];
    const char *program_name = argv[4];

    // Validate the algorithm specified
    if ((strcmp(algorithm, "rand") != 0) &&
        (strcmp(algorithm, "fifo") != 0) &&
        (strcmp(algorithm, "custom") != 0))
    {
        cerr << "ERROR: Unknown algorithm: " << algorithm << endl;
        exit(1);
    }

    // Validate the program specified
    program_f program = NULL;
    if (!strcmp(program_name, "sort"))
    {
        if (nframes < 2)
        {
            cerr << "ERROR: nFrames >= 2 for sort program" << endl;
            exit(1);
        }

        program = sort_program;
    }
    else if (!strcmp(program_name, "scan"))
    {
        program = scan_program;
    }
    else if (!strcmp(program_name, "focus"))
    {
        program = focus_program;
    }
    else
    {
        cerr << "ERROR: Unknown program: " << program_name << endl;
        exit(1);
    }

    // TODO - Any init needed
    frames_used.resize(nframes, false);



    // Create a virtual disk
    disk = disk_open("myvirtualdisk", npages);
    if (!disk)
    {
        cerr << "ERROR: Couldn't create virtual disk: " << strerror(errno) << endl;
        return 1;
    }

    // Create a page table
    struct page_table *pt = page_table_create(npages, nframes, page_fault_handler_random_eviction /* TODO - Replace with your handler(s)*/);
    if (!pt)
    {
        cerr << "ERROR: Couldn't create page table: " << strerror(errno) << endl;
        return 1;
    }

    // Run the specified program
    char *virtmem = page_table_get_virtmem(pt);
    program(virtmem, npages * PAGE_SIZE);

    // Clean up the page table and disk
    page_table_delete(pt);
    disk_close(disk);

    return 0;
}
