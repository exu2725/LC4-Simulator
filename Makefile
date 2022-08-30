all: trace

trace: LC4.o loader.o trace.o
	clang -g LC4.o loader.o trace.o -o trace
	
LC4.o: LC4.c
	clang -g -c LC4.c

loader.o: loader.c 
	clang -g -c loader.c

trace.o: trace.c
	clang -g -c trace.c

clean:
	rm -rf *.o

clobber: clean
	rm -rf trace
