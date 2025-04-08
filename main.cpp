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
#include <queue>
#include <stack>
#include <algorithm>
#include <fstream>

using namespace std;

// Prototype for test program
typedef void (*program_f)(char *data, int length);

// Number of physical frames
int num_frames;
int npages;

bool printflag = true;

// Pointer to disk for access from handlers
struct disk *disk = nullptr;
vector<bool> frames_used;

queue <int> fifo_queue; // FIFO queue for page replacement
vector <int> no_write;


int total_page_faults;
int total_disk_writes;
int total_disk_reads;

// Handler with random choice of eviction
void page_fault_handler_random_eviction(struct page_table *pt, int page) {
    if (printflag) {
        cout << "page fault on page #" << page << endl;
        // Print the page table contents
        cout << "Before ---------------------------" << endl;
        page_table_print(pt);
        cout << "----------------------------------" << endl;
    }

    // case for chnageing page to W
    int* bits = new int;
    int* framenumber = new int;
    page_table_get_entry(pt, page, framenumber, bits);
    if (*bits == PROT_READ ) // making a page dirty
    {
        page_table_set_entry(pt, page, *framenumber, PROT_READ | PROT_WRITE);
    } else if (*bits != PROT_READ) { // if the page needs to be alloced in Physcial mem
        total_page_faults++;
        // we need at add this to the page table 
        //check if frames are available
        bool already_allocated = false;
        for (int i = 0; i < page_table_get_nframes(pt); i++) {
            if (frames_used[i] == false) {
                // Found an empty frame
                frames_used[i] = true;
                disk_read(disk, page, page_table_get_physmem(pt) + (i * PAGE_SIZE));
                total_disk_reads++;
                page_table_set_entry(pt, page, i, PROT_READ);
                already_allocated = true;
                break;
            }
        }
        if (!already_allocated) {
            // No empty frames, we need to evict a page
            vector<int> pages_in_use;
            for (int i = 0; i < page_table_get_npages(pt); i++) {
                int* frame = new int;
                int* tempbits = new int;
                page_table_get_entry(pt, i, frame, tempbits);
                if (*tempbits != 0) {
                    pages_in_use.push_back(i);
                }
                delete tempbits;
                delete frame;
            }
            if (pages_in_use.empty()) {
                cerr << "ERROR: No pages in use to evict" << endl;
                exit(1);
            }
            int random_index = std::rand() % pages_in_use.size();
            int random_page = pages_in_use[random_index];

            int* replaced_page = new int;
            *replaced_page = random_page;

            int* replaced_page_bits = new int;
            int* replaced_frame = new int;

            page_table_get_entry(pt, random_page, replaced_frame, replaced_page_bits);

            if (*replaced_page_bits & PROT_WRITE){
                // Replaced page is dirty, we need to write it to the disk before replacing
                disk_write(disk, *replaced_page, page_table_get_physmem(pt) + (*replaced_frame * PAGE_SIZE));
                total_disk_writes++;
            } 
            // Replaced page is clean, we can just replace it
            disk_read(disk, page, page_table_get_physmem(pt) + (*replaced_frame * PAGE_SIZE));
            total_disk_reads++;
            page_table_set_entry(pt, *replaced_page, *framenumber, PROT_NONE);
            page_table_set_entry(pt, page, *replaced_frame, PROT_READ);
            //frames_used[*replaced_frame] = true;
            delete replaced_page;
            delete replaced_frame;
            delete replaced_page_bits;
        }
    }
    delete bits;
    delete framenumber;
    if (printflag) {
        // Print the page table contents
        cout << "After ----------------------------" << endl;
        page_table_print(pt);
        cout << "----------------------------------" << endl;
    }
   
}


// Handler with random choice of eviction
void page_fault_handler_fifo_eviction(struct page_table *pt, int page) {
    if (printflag) {
        cout << "page fault on page #" << page << endl;

        // Print the page table contents
        cout << "Before ---------------------------" << endl;
        page_table_print(pt);
        cout << "----------------------------------" << endl;
    }
    // case for chnageing page to W
    int* bits = new int;
    int* framenumber = new int;
    page_table_get_entry(pt, page, framenumber, bits);
    if (*bits == PROT_READ ) // making a page dirty
    {
        // std::cout << "Page is read only, changing to read/write" << endl;
        page_table_set_entry(pt, page, *framenumber, PROT_READ | PROT_WRITE);
    } else if (*bits != PROT_READ) { // if the page needs to be alloced in Physcial mem
        total_page_faults++;
        // we need at add this to the page table 
        //check if frames are available
        bool already_allocated = false;

        for (int i = 0; i < page_table_get_nframes(pt); i++) {
            if (frames_used[i] == false) {
                // Found an empty frame
                frames_used[i] = true;
                fifo_queue.push(page);
                disk_read(disk, page, page_table_get_physmem(pt) + (i * PAGE_SIZE));
                total_disk_reads++;
                page_table_set_entry(pt, page, i, PROT_READ);
                already_allocated = true;
                break;
            }
        }
        if (!already_allocated) {
            int* replaced_page = new int;
            int replaced_page_temp = fifo_queue.front();
            *replaced_page = replaced_page_temp;
            fifo_queue.pop();

            int* replaced_page_bits = new int;
            int* replaced_frame = new int;

            page_table_get_entry(pt, *replaced_page, replaced_frame, replaced_page_bits);
            

            if (*replaced_page_bits & PROT_WRITE){
                // Replaced page is dirty, we need to write it to the disk before replacing
                disk_write(disk, *replaced_page, page_table_get_physmem(pt) + (*replaced_frame * PAGE_SIZE));
                total_disk_writes++;
            } 
            // Replaced page is clean, we can just replace it
            disk_read(disk, page, page_table_get_physmem(pt) + (*replaced_frame * PAGE_SIZE));
            total_disk_reads++;
            page_table_set_entry(pt, *replaced_page, *framenumber, PROT_NONE);

            page_table_set_entry(pt, page, *replaced_frame, PROT_READ);
            //frames_used[*replaced_frame] = true;
            
            
            fifo_queue.push(page);
            delete replaced_page;
            delete replaced_frame;
            delete replaced_page_bits;
        }
    }
    delete bits;
    delete framenumber;
    if (printflag) {
        // Print the page table contents
        cout << "After ----------------------------" << endl;
        page_table_print(pt);
        cout << "----------------------------------" << endl;
    }
    
}



void page_fault_handler_custom_eviction(struct page_table *pt, int page) {
    
    if (printflag) {
        // Print the page table contents
        cout << "Before ---------------------------" << endl;
        page_table_print(pt);
        cout << "----------------------------------" << endl;
    }
   

    // case for chnageing page to W
    int* bits = new int;
    int* framenumber = new int;
    page_table_get_entry(pt, page, framenumber, bits);
    if (*bits == PROT_READ ) // making a page dirty
    {
        // std::cout << "Page is read only, changing to read/write" << endl;
        page_table_set_entry(pt, page, *framenumber, PROT_READ | PROT_WRITE);
        auto it = std::find(no_write.begin(), no_write.end(), page);
        if (it != no_write.end()) {
            no_write.erase(it);
        }
    } else if (*bits != PROT_READ) { // if the page needs to be alloced in Physcial mem
        // we need at add this to the page table 
        total_page_faults++;
        //check if frames are available
        bool already_allocated = false;

        for (int i = 0; i < page_table_get_nframes(pt); i++) {
            if (frames_used[i] == false) {
                // Found an empty frame
                frames_used[i] = true;
                disk_read(disk, page, page_table_get_physmem(pt) + (i * PAGE_SIZE));
                total_disk_reads++;
                no_write.push_back(page);
                page_table_set_entry(pt, page, i, PROT_READ);
                already_allocated = true;
                break;
            }
        }
        if (!already_allocated) {
            // No empty frames, we need to evict a page
            vector<int> pages_in_use;
            for (int i = 0; i < page_table_get_npages(pt); i++) {
                int* frame = new int;
                int* tempbits = new int;
                page_table_get_entry(pt, i, frame, tempbits);
                if (*tempbits != 0) {
                    pages_in_use.push_back(i);
                }
                delete tempbits;
                delete frame;
            }
            if (pages_in_use.empty()) {
                cerr << "ERROR: No pages in use to evict" << endl;
                exit(1);
            }
            int replaced_page_number;
            if (no_write.empty()) {
                int random_index = std::rand() % pages_in_use.size();
                replaced_page_number = pages_in_use[random_index];
            }
            else {
                // we have a page with only read permsions 
                replaced_page_number = no_write.back();
                no_write.pop_back();
            }

            int* replaced_page = new int;
            *replaced_page = replaced_page_number;

            int* replaced_page_bits = new int;
            int* replaced_frame = new int;

            page_table_get_entry(pt, *replaced_page, replaced_frame, replaced_page_bits);

            if (*replaced_page_bits & PROT_WRITE){
                // Replaced page is dirty, we need to write it to the disk before replacing
                disk_write(disk, *replaced_page, page_table_get_physmem(pt) + (*replaced_frame * PAGE_SIZE));
                total_disk_writes++;
            } 
            // Replaced page is clean, we can just replace it
            disk_read(disk, page, page_table_get_physmem(pt) + (*replaced_frame * PAGE_SIZE));
            total_disk_reads++;
            page_table_set_entry(pt, *replaced_page, *framenumber, PROT_NONE);

            page_table_set_entry(pt, page, *replaced_frame, PROT_READ);
            //frames_used[*replaced_frame] = true;
            auto it = std::find(no_write.begin(), no_write.end(), *replaced_page);
            if (it != no_write.end()) {
                no_write.erase(it);
            }
            no_write.push_back(page);
            delete replaced_page;
            delete replaced_frame;
            delete replaced_page_bits;
        }
    }
    delete bits;
    delete framenumber;
    if (printflag) {
        // Print the page table contents
        cout << "After ----------------------------" << endl;
        page_table_print(pt);
        cout << "----------------------------------" << endl;
    }
}


// TODO Handler with custom eviction

vector<int> mainfunc(int npages, int nframes, const char *algorithm, const char *program_name)
{
    std::cout << "USAGE\n";
    std::cout << "npages: " << npages;
    std::cout << " nframes: " << nframes ;
    std::cout << " algorithm: " << algorithm ;
    std::cout << " program: " << program_name ;
    std::cout << endl;

    total_page_faults = 0;
    total_disk_writes = 0;
    total_disk_reads = 0;

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
    } else if (!strcmp(program_name, "custom")){
        program = custom_program;
    }
    else
    {
        cerr << "ERROR: Unknown program: " << program_name << endl;
        exit(1);
    }

    // TODO - Any init needed
    frames_used.resize(nframes, false);
    cout << "Frames used: " << frames_used.size() << endl;
    for (int i = 0; i < nframes; i++) {
        frames_used[i] = false;
    }
    // make sure queue is empty 
    while (!fifo_queue.empty()) {
        fifo_queue.pop();
    }
    while (!no_write.empty()) {
        no_write.pop_back();
    }

    // Create a virtual disk
    disk = disk_open("myvirtualdisk", npages);
    if (!disk)
    {
        cerr << "ERROR: Couldn't create virtual disk: " << strerror(errno) << endl;
        exit(1);
    }

    // Create a page table
    page_fault_handler_t page_fault_handler = NULL;

    if (strcmp(algorithm, "rand") == 0)
    {
        page_fault_handler = page_fault_handler_random_eviction;
    }
    else if (strcmp(algorithm, "fifo") == 0)
    {
        page_fault_handler = page_fault_handler_fifo_eviction;
    }
    else if (strcmp(algorithm, "custom") == 0)
    {
        
        page_fault_handler = page_fault_handler_custom_eviction;
    }
    struct page_table *pt = page_table_create(npages,  nframes, page_fault_handler /* TODO - Replace with your handler(s)*/);
    if (!pt)
    {
        cerr << "ERROR: Couldn't create page table: " << strerror(errno) << endl;
        exit(1);
    }

    // Run the specified program
    char *virtmem = page_table_get_virtmem(pt);
    program(virtmem, npages * PAGE_SIZE);
    std::cout << "Total page faults: " << total_page_faults << endl;
    std::cout << "Total disk writes: " << total_disk_writes << endl;
    std::cout << "Total disk reads: " << total_disk_reads << endl;

    std::cout << "model: " << algorithm << endl;
    std::cout << "program: " << program_name << endl;
    std::cout << endl << endl;
    // Clean up the page table and disk
    page_table_delete(pt);
    disk_close(disk);
    
    return { total_page_faults, total_disk_writes, total_disk_reads };

}




int main(int argc, char *argv[]) {    
    if (argc == 5) {
        npages = atoi(argv[1]);
        num_frames = atoi(argv[2]);
        const char *algorithm = argv[3];
        const char *program_name = argv[4];

        vector<int> result = mainfunc(npages, num_frames, algorithm, program_name);
    }
    else if (argc == 2 && argv[1] == std::string("batch")) { // usage ./virtmem batch <npages> <num_frames> 
        printflag = false;
        std::cout << "__________BATCH MODE__________" <<endl;

        //vector <int> page_to_frame_ratio = { 1, 2, 3, 5, 10};
        //vector <int> page_to_frame_ratio = { 2, 10};

        vector <int> number_of_frames = {50, 20, 10, 5};

        vector <char*> algorithms = { "rand", "fifo", "custom" };
        vector <const char*> programs = { "sort", "scan", "focus", "custom" };
        std::ofstream file("output.txt");
        if (file.is_open()) {
            file << "npages, nframes, algorithm, program, pagefaults, diskwrites, diskreads" << endl;
            file.flush();
            file.close();
        } else {
            std::cout << "Unable to open file";
        }
        vector<vector<std::string>> results; // strings so we can put it all in one vector. We can chnage it back to ints later 
        // formatt of results. [npages, nframes, algorithm, program, pagefaults, diskwrites, diskreads]
        npages = 100;
        for (int i = 0; i < number_of_frames.size(); i++) {
            num_frames = number_of_frames[i];
            if (num_frames < 2) {
                continue;
            }
            for (int j = 0; j < programs.size(); j++) {
                const char *program_name = programs[j];
                for (int k = 0; k < algorithms.size(); k++) {
                    const char *algorithm = algorithms[k];
                    vector<int>result = mainfunc(npages, num_frames, algorithm, program_name);
                    vector<string> result_string = {
                        std::to_string(npages),
                        std::to_string(num_frames),
                        algorithm,
                        program_name,
                        std::to_string(result[0]),
                        std::to_string(result[1]),
                        std::to_string(result[2])
                    };
                    results.push_back(result_string);
                    std::ofstream file("output.txt", std::ios::app);
                    if (file.is_open()) {
                        file << npages << ", " << num_frames << ", " << algorithm << ", " << program_name << ", " << result[0] << ", " << result[1] << ", " << result[2] << endl;
                        file.flush();
                        file.close();
                    } else {
                        std::cout << "Unable to open file";
                    }
                }
            }            
        }
        
        std::cout << "__________RESULT__________" <<endl;
        for (int i = 0; i < results.size(); i++) {
            for (int j = 0; j < results[i].size(); j++) {
                std::cout << results[i][j] << " ";
            }
            std::cout << endl;
        }
        //write result to a file
        std::ofstream outfile("results.txt");
        if (outfile.is_open()) {
            for (int i = 0; i < results.size(); i++) {
                for (int j = 0; j < results[i].size(); j++) {
                    outfile << results[i][j] << ", ";
                }
                outfile << endl;
            }
            outfile.close();
        } else {
            std::cout << "Unable to open file";
        }
    }
}