




all: clean
	g++ -g -Wall -Wunused -Wextra -Werror -std=c++14 -o dynarec dynarec.cpp


clean:
	rm -f dynarec

