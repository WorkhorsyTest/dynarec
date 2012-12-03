

all: clean
	nasm -o out -f bin test_push_pop.asm
	#nasm -o out.o -f elf out.asm
	#ld -s -o out out.o
	ndisasm out

clean:
	rm -f out.o
	rm -f out


