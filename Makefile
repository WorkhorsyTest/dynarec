




all: clean
	g++ -g -Wall -Wunused -Wextra -Werror -o dynarec dynarec.cpp


clean:
	rm -f dynarec

