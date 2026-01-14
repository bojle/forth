#all: forth.s
#	clang -o forth forth.s

forth: forth.cpp
	clang++ -g -o forth forth.cpp -std=c++23

test: forth
	python3 test.py test/*.f
