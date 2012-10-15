




all: clean
	g++ -g -Wall -Werror -o dynarec dynarec.cpp


clean:
	rm -f dynarec

