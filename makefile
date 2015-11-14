all:	bin/PF.o	bin/RM.o	bin/SM.o	bin/parser.o	bin/CREATE	makefile	bin/DROP 	bin/SHOW	bin/USE	

bin/CREATE:	CREATE.cpp	bin/RM.o	bin/PF.o
	g++ bin/PF.o bin/SM.o bin/RM.o CREATE.cpp -o bin/CREATE

bin/DROP:	DROP.cpp	bin/RM.o	bin/PF.o
	g++ bin/PF.o bin/RM.o DROP.cpp -o bin/DROP

bin/SHOW:	SHOW.cpp	bin/RM.o	bin/PF.o
	g++ bin/PF.o bin/RM.o SHOW.cpp -o bin/SHOW

bin/USE:	USE.cpp		bin/RM.o	bin/PF.o	bin/SM.o	bin/parser.o
	g++ bin/PF.o bin/RM.o bin/SM.o bin/parser.o USE.cpp -o bin/USE

bin/parser.o:	parser.h	parser.cpp	system_manage.h
	g++ -c parser.cpp -o bin/parser.o

bin/SM.o:	system_manage.h	system_manage.cpp
	g++ -c system_manage.cpp -o bin/SM.o

bin/RM.o: pages_file.h record_manager.h record_manager.cpp pf_defines.h
	g++ -c record_manager.cpp -o bin/RM.o

bin/PF.o: pages_file.h pages_file.cpp pf_defines.h
	g++ -c pages_file.cpp -o bin/PF.o	

clean:
	rm -r bin/*
	rm *~
