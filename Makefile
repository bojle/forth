#all: forth.s
#	clang -o forth forth.s

forth: forth.cpp
	clang++ -o forth forth.cpp -std=c++23
