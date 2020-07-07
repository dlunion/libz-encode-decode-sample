



run : all
	@echo run program
	@./run

all : main.o
	@echo linking 
	@g++ -o run $^ -lz
	@echo linking done

main.o : main.cpp
	@echo compiling $<
	@g++ -c $< -o $@ -std=c++11

