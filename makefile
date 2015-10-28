PF.o: pages_file.h pages_file.cpp pf_defines.h
	g++ -c pages_file.cpp -o bin/PF.o	

clear:
	rm bin/*
	rm *~
