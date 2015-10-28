RM.o: pages_file.h record_manager.h record_manager.cpp pf_defines.h
	g++ -c record_manager.cpp -o bin/RM.o

PF.o: pages_file.h pages_file.cpp pf_defines.h
	g++ -c pages_file.cpp -o bin/PF.o	

clear:
	rm bin/*
	rm *~
