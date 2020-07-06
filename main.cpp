#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <iomanip>

#define INPUTFILENAME "addresses.txt"

#define PAGE_SIZE 256
#define FRAME_SIZE 256
#define VIRTUAL_MEMORY_SIZE 256
#define MEMORY_SIZE 256
#define TLB_SIZE 16

using namespace std;

class BackingStore {
public:
    FILE* fp;

    BackingStore() {
        cout << "A new backing store has been created !!! " << endl;
        fp = fopen("BACKING_STORE.bin", "rb");
        if (!fp) {
            cerr << "Error while opening BackingStore.bin" << endl;
            //getchar(); <s//for debugging
            exit(-1);
        }
        else {
            cout <<"File BACKING_STORE opened successfully !!! " <<endl;
        }
    }

    ~BackingStore() {
        fclose(fp);
        cout << "BackingStore was destroyed. \n";
    }

    void readPage(int page, char* target) {             // Read the file from the binary and save our tags to the target
        if (fseek(fp, page*PAGE_SIZE, SEEK_SET)<0) {    
            cerr << "Warning: fseek failed!" << endl;
        }
        if (fread(target, PAGE_SIZE, 1, fp)<0) {        //we read 256 bytes at a time(1 PAGE_SIZE)
            cerr << "Warning: fread failed!" << endl;
        }
    }
};

class VirtualMemoryRow {
public:
    int frame;
    bool empty;

    VirtualMemoryRow() : frame(-1), empty(true) {
    }
};

class VirtualMemory {
public:
    VirtualMemoryRow pinakas[VIRTUAL_MEMORY_SIZE];

    VirtualMemory() {
        cout << "A new virtual memory has been created !!! " << endl;
    }

    ~VirtualMemory() {
        cout << "Virtual memory was destroyed." << endl;
    }

    bool contains(int page) {           // returns true if VM contains the page
        if (!pinakas[page].empty) {
            return true;
        } else {
            return false;
        }
    }

    void insert(int page, int frame) {  //insert frame in VM table in position pinakas[page] // we always have space in the BM board to import
        pinakas[page].empty = false;
        pinakas[page].frame = frame;
    }

    int getFrameNumber(int page) {      // returns the framenumber if page exists or -1
        if (pinakas[page].empty) {
            return -1;}
        else {
            return pinakas[page].frame;
        }
    }
};

class PhysicalMemoryRow {
public:
    char data[FRAME_SIZE];  // frame size
    int age;                //initializing for lru
    bool empty;

    PhysicalMemoryRow() : age(0), empty(true) {
    }
};

class PhysicalMemory {
public:
    PhysicalMemoryRow pinakas[MEMORY_SIZE];
    BackingStore backingstore;      //Physical memory contains the backing_store
    int max_age;                    // maximum known age ...for lru

    PhysicalMemory():max_age(0) {
        cout << "A new physical memory was created !!! " << endl;
    }

    ~PhysicalMemory() {
        cout << "Physical memory has been destroyed." << endl;
    }

    int insert(int page) {              // returns framenumber of the page inserted //
        for (int i=0;i<MEMORY_SIZE;i++) {
            if (pinakas[i].empty) {
                pinakas[i].empty = false;
                backingstore.readPage(page, pinakas[i].data);
                return i;   // returns framenumber ...
            }
        }
        // lru code  //we think we have an infinite memory
        int minage = pinakas[0].age;
        int position = 0;
        for (int i=1;i<MEMORY_SIZE;i++) {
             if (pinakas[i].age < minage) {
                minage = pinakas[i].age;
                position = i;
             }
        }
        backingstore.readPage(page, pinakas[position].data);
        return position; // returns framenumber after lru
    }

    char getByte(int frame, int offset) {
        if (pinakas[frame].empty) {
            return -1;
        } else {
            return pinakas[frame].data[offset];
        }
    }

    void updateAge(int frame) { 
        max_age++;
        pinakas[frame].age = max_age;
    }
};

class TranslationLookAsideBufferRow {
public:
    int page;
    int frame;
    int age; //for lru
    bool empty;

    TranslationLookAsideBufferRow():page(-1), frame(-1), age(0), empty(true) {
    }
};

class TranslationLookAsideBuffer {
public:
    int max_age;    // for lru
    TranslationLookAsideBufferRow pinakas[TLB_SIZE];

    TranslationLookAsideBuffer() : max_age(0) {
        cout << "A new translation lookaside buffer has been created !!! " << endl;
    }

    ~TranslationLookAsideBuffer() {
        cout << "Translation lookaside buffer was destroyed." << endl;
    }

    bool contains(int page) {      
        for (int i=0;i<16;i++) {
            if (pinakas[i].empty == false && pinakas[i].page == page) {
                return true;
            }
        }
        return false;
    }

    int getFrameNumber(int page) { //returns the frame number -1
        for (int i=0;i<16;i++) {
            if (pinakas[i].empty == false && pinakas[i].page == page) {
                return pinakas[i].frame;
            }
        }
        return -1;
    }

    void updateAge(int page) { 
        for (int i=0;i<16;i++) {
            if (pinakas[i].empty == false && pinakas[i].page == page) {
                max_age++;
                pinakas[i].age = max_age;
            }
        }
    }

    void insert(int page, int frame) {  //insert doesnot need extra input do it in here
        for (int i=0;i<16;i++) {
            if (pinakas[i].empty) {         // empty cell found ...
                pinakas[i].page = page;
                pinakas[i].frame = frame;
                pinakas[i].empty = false;
                max_age++;
                pinakas[i].age = max_age;
                return;
            }
        }
        
        
        // lru code:
        int minage = pinakas[0].age;
        int position = 0;
        for (int i=1;i<16;i++) {
             if (pinakas[i].age < minage) {
                minage = pinakas[i].age;
                position = i;
             }
        }
        pinakas[position].page = page;
        pinakas[position].frame = frame;
        max_age++;
        pinakas[position].age = max_age;
    }
};

class Simulation {
public:
    VirtualMemory virtualMemory;
    PhysicalMemory physicalMemory;
    TranslationLookAsideBuffer tlb;

    Simulation() {
        cout << "A new simulation has been created !!!" << endl;
    }

    ~Simulation() {
        cout << "Simulation was destroyed." << endl;
    }

    void run() {
        ifstream in(INPUTFILENAME);     //File open for reading
        int temp;                       //for the virtal address from address.txt
        int counter = 0;                //counter for all integers i read
        int page;                       //the page i calculate from virtual address
        int offset;                     //the offset i calculate from the virtual address
        int tlb_hits = 0;
        int vm_hits = 0;
        int tlb_misses = 0;
        int pf = 0;
        char byte;

        cout << "\nSimulation is starting ... \n" << endl;
        cout << "Physical Memory " << MEMORY_SIZE << endl;
        cout << "Virtual  Memory " << VIRTUAL_MEMORY_SIZE << endl;
        cout << "TLB rows        " << TLB_SIZE<<"\n"<<endl;

        if (!in) {
            cerr << "Cannot open file " << INPUTFILENAME << endl;
            return;
        } else {
            cout << "File " << INPUTFILENAME << " opened successfully !!!\n" << endl;
        }
        while ((in>>temp) && !in.eof()) {    //scan all the file
            page = (temp & 0x0000FF00) >> 8; //logican and  to read the page
            offset = (temp & 0x000000FF);   //logican and  to read the offset
            counter ++;
           
            if (tlb.contains(page)) {  
                int frame = tlb.getFrameNumber(page); 
                int physicalAddress = frame*FRAME_SIZE + offset; 
                tlb.updateAge(page); 
                //physicalMemory.updateAge(frame); /// redundant, it can be erased, you can add it if we made a lot of physical memory
                tlb_hits++;
                byte = physicalMemory.getByte(frame, offset);
                cout << setw(4) << counter <<" V.Address = " << setw(5)<< temp  <<" ,P.Address = " << setw(5) << physicalAddress << " ,Byte = " << setw(4) <<(int)byte <<setw(9)<< " hit@TLB " << " ,Page = " << setw(3) << page << " ,Offset = " << setw(3) << offset << " ,Frame = " << setw(3) << frame <<endl;
            } else if (virtualMemory.contains(page)) { //
                int frame = virtualMemory.getFrameNumber(page); //vriskw to frame apo to VM table
                int physicalAddress = frame*FRAME_SIZE + offset; //calculating physical address
                tlb.insert(page, frame);    
                //physicalMemory.updateAge(frame);  /// it can be erased, you can add it to the physical memory
                vm_hits++;
                tlb_misses++;
                byte = physicalMemory.getByte(frame, offset); //pass byte through physical memory
                cout << setw(4) << counter <<" V.Address = " << setw(5)<< temp  <<" ,P.Address = " << setw(5) << physicalAddress << " ,Byte = " << setw(4) <<(int)byte << setw(9) <<" hit@VM " << " ,Page = " << setw(3) << page << " ,Offset = " << setw(3) << offset << " ,Frame = " << setw(3) << frame <<endl;
            } else { 
                int frame = physicalMemory.insert(page); //insert it into the physical memory from the Backing store
                int physicalAddress = frame*FRAME_SIZE + offset; //calculate the physical address
                virtualMemory.insert(page, frame);  //update tin VM
                tlb.insert(page, frame);            //update ton tlb
                //physicalMemory.updateAge(frame);    /// it can be erased, you can add it to the physical memory
                tlb_misses++;
                pf++;
                byte = physicalMemory.getByte(frame, offset); //update byte through natural memory
                cout << setw(4) << counter <<" V.Address = " << setw(5)<< temp  <<" ,P.Address = " << setw(5) << physicalAddress << " ,Byte = " << setw(4) <<(int)byte << setw(9)<<" MISS " << " ,Page = " << setw(3) << page << " ,Offset = " << setw(3) << offset << " ,Frame = " << setw(3) << frame <<endl;
            }
        }

        in.close();

        cout << "\n----------------------------------------- Printing Statistics ----------------------------------------\n" << endl;
        cout << "       References : " <<counter <<"\n"<<endl;
        cout << "       Page faults: " << setw(4) << pf << " (" << (pf*100.0/counter) << "%)\n" << endl;
        cout << "       TLB hits   : " << setw(4) << tlb_hits << " (" << (tlb_hits*100.0/counter) << "%)\n" << endl;
        cout << "       TLB misses : " << setw(4) << tlb_misses << " (" << (tlb_misses*100.0/counter) << "%)\n" << endl;
        cout << "       VM hits    : " << setw(4) << vm_hits << " (" << (vm_hits*100.0/counter) << "%)\n" << endl;
    }
};

int main() {
    Simulation simulation;
    simulation.run();
    return 0;
}
