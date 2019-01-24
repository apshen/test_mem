all: test_mem

test_mem: test_mem.c
	gcc -Wall test_mem.c -o test_mem

clean:
	rm test_mem
