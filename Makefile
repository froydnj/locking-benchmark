CXX := g++

all: fairness

fairness: fairness.cpp
	$(CXX) -std=c++14 -O2 -o $@ $< -lpthread

clean:
	rm -f fairness
