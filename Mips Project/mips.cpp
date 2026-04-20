#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <map>
#include <fstream>
#include <algorithm>
#include <cstring>

using namespace std;

// --- Data Structures for Pipeline Registers ---
struct IF_ID { string instr = "NOP"; int pc = 0; };
struct ID_EX { 
    string op = "NOP"; int rs = 0, rt = 0, rd = 0, imm = 0, v1 = 0, v2 = 0; 
    bool regWrite = false, memRead = false, memWrite = false, memToReg = false;
};
struct EX_MEM { 
    string op = "NOP"; int aluRes = 0, rtVal = 0, rd = 0; 
    bool regWrite = false, memRead = false, memWrite = false, memToReg = false;
};
struct MEM_WB { 
    string op = "NOP"; int data = 0, rd = 0; 
    bool regWrite = false;
};

class MIPSProcessor {
private:
    int regs[32] = {0};
    int mem[1024] = {0};
    vector<string> imem; // Instruction Memory
    int pc = 0, cycle = 0;
    IF_ID ifid; 
	ID_EX idex; 
	EX_MEM exmem; 
	MEM_WB memwb;

public:
    MIPSProcessor(vector<string> p) : imem(p) {}

    void step() {
		bool stall = false; //For Load-Use Hazard
		
        // 5. WRITE BACK (WB)
        if (memwb.regWrite && memwb.rd != 0) regs[memwb.rd] = memwb.data;

        // 4. MEMORY (MEM)
        memwb.op = exmem.op; 
		memwb.rd = exmem.rd; 
		memwb.regWrite = exmem.regWrite;
        if (exmem.memRead) memwb.data = mem[exmem.aluRes / 4];
        else if (exmem.memWrite) mem[exmem.aluRes / 4] = exmem.rtVal;
        else memwb.data = exmem.aluRes;


        // 3. EXECUTE (EX)
		
        exmem.op = idex.op; 
		exmem.rd = idex.rd; 
		exmem.rtVal = idex.v2;
        exmem.regWrite = idex.regWrite; 
		exmem.memRead = idex.memRead; 
		exmem.memWrite = idex.memWrite;
        
        if (idex.op == "ADD") exmem.aluRes = idex.v1 + idex.v2;
        else if (idex.op == "ADDI") exmem.aluRes = idex.v1 + idex.imm;
        else if (idex.op == "SUB") exmem.aluRes = idex.v1 - idex.v2;
        else if (idex.op == "MUL") exmem.aluRes = idex.v1 * idex.v2;
        else if (idex.op == "AND") exmem.aluRes = idex.v1 & idex.v2;
        else if (idex.op == "OR")  exmem.aluRes = idex.v1 | idex.v2;
        else if (idex.op == "SLL") exmem.aluRes = idex.v2 << idex.imm;
        else if (idex.op == "SRL") exmem.aluRes = idex.v2 >> idex.imm;
        else if (idex.op == "LW" || idex.op == "SW") exmem.aluRes = idex.v1 + idex.imm;

		


        // 2. DECODE (ID)
        idex = ID_EX(); // Clear signals
		
		if (idex.op == "NOP" && exmem.op == "LW") {
			stringstream ss_haz(ifid.instr);
			string h_op;
			int h_rd, h_rs, h_rt;
			ss_haz >> h_op >> h_rd >> h_rs >> h_rt;
			
			if (exmem.rd != 0 && (exmem.rd == h_rs || exmem.rd == h_rt)) {
				idex.op = "NOP";
				pc--;
				return;
			}
		}
		
		
        if (ifid.instr != "NOP") {
            stringstream ss(ifid.instr); string op; ss >> op;
            idex.op = op;
            if (op == "ADD" || op == "SUB" || op == "MUL" || op == "AND" || op == "OR") {
                ss >> idex.rd >> idex.rs >> idex.rt;
                idex.v1 = regs[idex.rs]; 
				idex.v2 = regs[idex.rt]; 
				idex.regWrite = true;
            } else if (op == "ADDI") {
                ss >> idex.rd >> idex.rs >> idex.imm;
                idex.v1 = regs[idex.rs]; 
				idex.regWrite = true;
			} else if (op == "SLL" || op == "SRL") {
				ss >> idex.rd >> idex.rt >> idex.imm;
				idex.v2 = regs[idex.rt];
				idex.regWrite = true;
			} else if (op == "LW") {
                ss >> idex.rd >> idex.imm >> idex.rs;
                idex.v1 = regs[idex.rs]; 
				idex.regWrite = true; 
				idex.memRead = true;
            } else if (op == "SW") {
                ss >> idex.rt >> idex.imm >> idex.rs;
                idex.v1 = regs[idex.rs]; 
				idex.v2 = regs[idex.rt]; 
				idex.memWrite = true;
            } else if (op == "BEQ") {
                ss >> idex.rs >> idex.rt >> idex.imm;
                if (regs[idex.rs] == regs[idex.rt]) pc += idex.imm; // Simplified branch
            } else if (op == "BNE") {
				ss >> idex.rs >> idex.rt >> idex.imm;
				if (regs[idex.rs] != regs[idex.rt]) pc += idex.imm;
			} else if (op == "J") {
                ss >> idex.imm; pc = idex.imm;
            }
        }
		
		//DATA HAZARD PREVENTION
		if (exmem.regWrite && exmem.rd != 0 && exmem.rd == idex.rs) idex.v1 = exmem.aluRes;
		else if (memwb.regWrite && memwb.rd != 0 && memwb.rd == idex.rs) idex.v1 = memwb.data;
		if (exmem.regWrite && exmem.rd != 0 && exmem.rd == idex.rt) idex.v2 = exmem.aluRes;
		else if (memwb.regWrite && memwb.rd != 0 && memwb.rd == idex.rt) idex.v2 = memwb.data;


        // 1. FETCH (IF)
        if (pc < imem.size()) { 
			ifid.instr = imem[pc]; 
			ifid.pc = pc; 
			pc++; 
		} else { ifid.instr = "NOP"; }
        cycle++;
    }

    void printState() {
		const char* dataCol = "\033[36m"; //Cyan
		const char* addrCol = "\033[35m"; //Magenta
		const char* cyclCol = "\033[32m"; //Green
		const char* moduCol = "\033[33m";
		const char* reset = "\033[0m";
		
		
        cout << cyclCol << "\n\n\n\n[Cycle " << cycle << "]" << reset << "\n\tPC: " << pc << endl;
		
		// Print the Pipeline State Registers
		cout << moduCol << "\n\nInstruction Fetch -> Decode:" << reset;
		cout << "\n\tOp Code:\t" << ifid.instr;
		
		cout << moduCol << "\n\nInstruction Decode - > Execution:" << reset;
		cout << "\n\tOp:\t\t" << idex.op;
		cout << "\n\tRS:\t\t" << idex.rs << "\tRT:\t\t" << idex.rt << "\tRD:\t" << idex.rd;
		cout << "\n\tRS Val:\t\t" << idex.v1 << "\tRT Val:\t\t" << idex.v2 << "\tImm:\t" << idex.imm; 
		cout << "\n\tregWrite:\t" << idex.regWrite << "\tmemRead:\t" << idex.memRead << "\n\tmemWrite:\t" << idex.memWrite << "\tmemToReg:\t" << idex.memToReg;

		cout << moduCol << "\n\nExecution - > Memory:" << reset;
		cout << "\n\tOp:\t\t" << exmem.op;
		cout << "\n\tALU Res:\t" << exmem.aluRes << "\tRT Val:\t\t" << exmem.rtVal << "\tRD:\t" << exmem.rd;; 
		cout << "\n\tregWrite:\t" << exmem.regWrite << "\tmemRead:\t" << exmem.memRead << "\n\tmemWrite:\t" << exmem.memWrite << "\tmemToReg:\t" << exmem.memToReg;

		cout << moduCol << "\n\nMemory - > Write Back:" << reset;
		cout << "\n\tOp:\t\t" << memwb.op;
		cout << "\n\tData:\t\t" << memwb.data << "\tRD:\t" << memwb.rd;
		cout << "\n\tregWrite:\t" << memwb.regWrite;
		
		// Print the register file
		cout << moduCol << "\n\nRegister File:" << reset;
		for (int i = 0; i < 32; i++){
			if (i%8==0){
				char addr_index[5];
				sprintf(addr_index, "0x%02x", i);
				cout << addrCol << "\n\t" << addr_index << ":\t" << reset;
			}
			char buf[9];
			sprintf(buf, "%08x", regs[i]);
			int j = 0;
			while (j < 7 && buf[j] == '0') j++;
			cout.write(buf, j);
			cout << dataCol;
			cout.write(buf + j, strlen(buf) - j);
			cout << reset << "   ";
		}
		
		// Print the contents of memory
		cout << moduCol << "\n\nMemory:" << reset;
		for (int i = 0; i < 1024; i++){
			if (i%16 == 0){
				char addr_index[6];
				sprintf(addr_index, "0x%03x", i);
				cout << addrCol << "\n\t" << addr_index << ":\t" << reset;
			}
			char buf[9];
			sprintf(buf, "%08x", mem[i]);
			int j = 0;
			while (j < 7 && buf[j] == '0') j++;
			cout.write(buf, j);
			cout << dataCol;
			cout.write(buf + j, strlen(buf) - j);
			cout << reset << "   ";
		}
    }

    bool isDone() { return (pc >= imem.size() && ifid.instr == "NOP" && idex.op == "NOP" && exmem.op == "NOP" && memwb.op == "NOP"); }
};


/*
struct ID_EX { 
    string op = "NOP"; int rs = 0, rt = 0, rd = 0, imm = 0, v1 = 0, v2 = 0; 
    bool regWrite = false, memRead = false, memWrite = false, memToReg = false;
};
struct EX_MEM { 
    string op = "NOP"; int aluRes = 0, rtVal = 0, rd = 0; 
    bool regWrite = false, memRead = false, memWrite = false, memToReg = false;
};
struct MEM_WB { 
    string op = "NOP"; int data = 0, rd = 0; 
    bool regWrite = false;
};
*/

int main(int argc, char*argv[]) {
	if (argc < 2) {
		cout << "Usage: " << endl;
		cout << "   ./file_process <mips_file> [debug]" << endl;
		cout << "\n  Mips file should be any file with valid MIPS assembly code." << endl;
		cout << "Debug can range from 0-2:";
		cout << "\n   0: No Debug";
		cout << "\n   1: Print state inbetween each cycle";
		cout << "\n   2: Stop inbetween each state and wait enter key";
		return 0;
	}
	
	string instFile = argv[1];
	int debug = argc >= 3 ? atoi(argv[2]) : 0;
	
	ifstream file(instFile);
	
	//file.open(instFile);
	vector<string> program;
	string line;
	
	if (!file.is_open()) {
		cerr << "Could not open input file " << instFile << ". Exiting ..." << endl;
		exit(0);
	}
	
	while (getline(file, line)) { // Removes $ and , to make parsing easier
		size_t commentPos = line.find('#');
		if (commentPos != string::npos) {
			line = line.substr(0, commentPos);
		}
		
		for (char &c : line) {
			if (c == ',' || c == '$' || c == '(' || c == ')') c = ' ';
		}
		
		if (!line.empty() && line.find_first_not_of(" \t\n\v\f\r") != string::npos) {
            program.push_back(line);
			//cout << line << endl;
        }
	}
	file.close();
	
	
	
    MIPSProcessor sim(program);
    while (!sim.isDone()) {
		sim.step();
		if (debug) { sim.printState(); cout << endl; }
		if (debug == 2) cin.get();
	}
	if (!debug) sim.printState();
	cout << "\n\n\033[32mSimulation Complete!\033[0m";
    return 0;
}