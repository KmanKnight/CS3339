Compilation:
	g++ mips.cpp -o {executable_name}

Run:
	./{executable_name} {assem_file} [debug]
	
	assem_file: Any valid MIPS file
	debug: Level of debug (Default 0)
		0:  No debug
		1:  Prints state registers after every cycle
		2:  Stops and waits for enter keys after every cycle
