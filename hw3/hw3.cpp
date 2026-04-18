#include <iostream>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <cmath>

using namespace std;

class Entry {
public:
    Entry() : valid(false), tag(0), ref(0) {} // Initialize as empty 
    ~Entry() {}

    void set_tag(int _tag) { tag = _tag; }
    int get_tag() { return tag; }

    void set_valid(bool _valid) { valid = _valid; }
    bool get_valid() { return valid; }

    void set_ref(int _ref) { ref = _ref; }
    int get_ref() { return ref; }

private:  
    bool valid;
    unsigned tag;
    int ref; // Used for LRU tracking
};

class Cache {
public:
    Cache(int _num_entries, int _assoc) : num_entries(_num_entries), assoc(_assoc) {
        num_sets = num_entries / assoc; // 
        entries = new Entry*[num_sets];
        for (int i = 0; i < num_sets; i++) {
            entries[i] = new Entry[assoc];
        }
    }

    ~Cache() {
        for (int i = 0; i < num_sets; i++) delete[] entries[i];
        delete[] entries;
    }

    int get_index(unsigned long addr) { return addr % num_sets; }
    int get_tag(unsigned long addr) { return addr / num_sets; }

    bool process_reference(unsigned long addr, int timer) {
        int index = get_index(addr);
        int tag = get_tag(addr);
        
        // Search for Hit
        for (int i = 0; i < assoc; i++) {
            if (entries[index][i].get_valid() && entries[index][i].get_tag() == tag) {
                entries[index][i].set_ref(timer); // Update LRU 
                return true;
            }
        }

        // Handle Miss (Update Cache)
        int lru_way = 0;
        int min_ref = timer + 1;

        for (int i = 0; i < assoc; i++) {
            if (!entries[index][i].get_valid()) {
                lru_way = i;
                break;
            }
            if (entries[index][i].get_ref() < min_ref) {
                min_ref = entries[index][i].get_ref();
                lru_way = i;
            }
        }

        entries[index][lru_way].set_valid(true);
        entries[index][lru_way].set_tag(tag);
        entries[index][lru_way].set_ref(timer);
        return false;
    }

private:
    int assoc;
    unsigned num_entries;
    int num_sets;
    Entry **entries;
};

int main(int argc, char* argv[]) {
    if (argc != 4) { // Requires 3 arguments [cite: 15]
        cout << "Usage: ./cache_sim num_entries associativity input_file" << endl;
        return 0;
    }

    int num_entries = atoi(argv[1]);
    int associativity = atoi(argv[2]);
    string input_filename = argv[3];

    Cache myCache(num_entries, associativity);
    
    ifstream input(input_filename);
    ofstream output("cache_sim_output"); // Exact output name required 

    if (!input.is_open()) {
        cerr << "Could not open input file." << endl;
        exit(0);
    }

    unsigned long addr;
    int timer = 0;
    while (input >> addr) {
        timer++;
        bool is_hit = myCache.process_reference(addr, timer);
        // Required format: [ADDR]: [HIT or MISS] [cite: 25, 26]
        output << addr << ": " << (is_hit ? "HIT" : "MISS") << endl;
    }

    input.close();
    output.close();
    return 0;
}