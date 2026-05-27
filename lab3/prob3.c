#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


#define PAGE_SIZE 256
#define PHYSICAL_MEMORY_SIZE 65536  // 2^16
#define TLB_SIZE 16
#define PAGE_TABLE_SIZE 256
#define FRAME_SIZE PAGE_SIZE
#define BACKING_STORE_FILE "BACKING_STORE.bin" 


typedef struct {
    int page_number;
    int frame_number;
} TLBEntry;


char physical_memory[PHYSICAL_MEMORY_SIZE];
int page_table[PAGE_TABLE_SIZE];
TLBEntry tlb[TLB_SIZE];
int tlb_index = 0;
int page_fault_count = 0;
int tlb_hit_count = 0;
int next_available_frame = 0; 
int tlb_full = 0; 
int total_addresses = 0;

int translate_address(int logical_address);
int get_frame_from_page_table(int page_number);
int get_frame_from_tlb(int page_number);
void add_to_tlb(int page_number, int frame_number);
int handle_page_fault(int page_number);
void load_page_from_backing_store(int page_number, int frame_number);

int8_t get_signed_byte(int physical_address) {
    return (int8_t)physical_memory[physical_address];
}

int main(int argc, char* argv[]) {


    if (argc != 2) {
        fprintf(stderr, "Usage: %s <addresses.txt>\n", argv[0]);  
        return 1;
    }

    const char* addresses_file = argv[1]; 


    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        page_table[i] = -1; 
    }


    for (int i = 0; i < TLB_SIZE; i++) {
        tlb[i].page_number = -1;  
        tlb[i].frame_number = -1;  
    }


    FILE* addr_fp = fopen(addresses_file, "r");
    if (addr_fp == NULL) {
        perror("Error opening addresses file");
        return 1;
    }

    char line[10];
    int logical_address;

    while (fgets(line, sizeof(line), addr_fp) != NULL) {
        logical_address = atoi(line); 
        total_addresses++; 

        int physical_address = translate_address(logical_address);

        int8_t signed_byte = get_signed_byte(physical_address);

        printf("Logical address: %d, Physical address: %d, Value: %d\n",
            logical_address, physical_address, signed_byte);
    }

    fclose(addr_fp);


    double page_fault_rate = (double)page_fault_count / total_addresses * 100.0;
    double tlb_hit_rate = (double)tlb_hit_count / total_addresses * 100.0;

    printf("Page Fault Rate: %.2f%%\n", page_fault_rate);
    printf("TLB Hit Rate: %.2f%%\n", tlb_hit_rate);

    return 0;
}


int translate_address(int logical_address) {
    int page_number = (logical_address >> 8) & 0xFF; 
    int offset = logical_address & 0xFF;            

    int frame_number = get_frame_from_tlb(page_number);

    if (frame_number != -1) {

        tlb_hit_count++;
    }
    else {

        frame_number = get_frame_from_page_table(page_number);

        if (frame_number == -1) {

            frame_number = handle_page_fault(page_number);
        }


        add_to_tlb(page_number, frame_number);
    }

    int physical_address = (frame_number << 8) | offset;
    return physical_address;
}


int get_frame_from_tlb(int page_number) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].page_number == page_number) {
            return tlb[i].frame_number;
        }
    }
    return -1;
}


int get_frame_from_page_table(int page_number) {
    return page_table[page_number];
}


void add_to_tlb(int page_number, int frame_number) {
    tlb[tlb_index].page_number = page_number;
    tlb[tlb_index].frame_number = frame_number;
    tlb_index = (tlb_index + 1) % TLB_SIZE; 

    if (tlb_index == 0) {
        tlb_full = 1;
    }

}


int handle_page_fault(int page_number) {
    page_fault_count++;


    int frame_number = next_available_frame;


    if (next_available_frame >= PHYSICAL_MEMORY_SIZE / PAGE_SIZE) {


        int page_to_replace = -1;
        for (int i = 0; i < PAGE_TABLE_SIZE; ++i) {
            if (page_table[i] != -1) { 
                page_to_replace = i;
                break; 
            }
        }

        if (page_to_replace != -1) {

            for (int i = 0; i < TLB_SIZE; ++i) {
                if (tlb[i].page_number == page_to_replace) {
                    tlb[i].page_number = -1;
                    tlb[i].frame_number = -1;
                }
            }

           
            frame_number = page_table[page_to_replace];
            page_table[page_to_replace] = -1;
        }
        else {
            fprintf(stderr, "Error: No page found to replace during FIFO.\n");
            exit(1);
        }
    }
    else {
        frame_number = next_available_frame;
        next_available_frame++;
    }



    load_page_from_backing_store(page_number, frame_number);


    page_table[page_number] = frame_number;

    return frame_number;
}


void load_page_from_backing_store(int page_number, int frame_number) {
    FILE* backing_store_fp = fopen(BACKING_STORE_FILE, "rb"); 
    if (backing_store_fp == NULL) {
        perror("Error opening backing store file");
        exit(1); 
    }

    fseek(backing_store_fp, page_number * PAGE_SIZE, SEEK_SET);
    fread(physical_memory + (frame_number * PAGE_SIZE), sizeof(char), PAGE_SIZE, backing_store_fp); 

    fclose(backing_store_fp);
}