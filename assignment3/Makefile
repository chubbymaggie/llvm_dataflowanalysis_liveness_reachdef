all: live.so reach.so

CXXFLAGS = -rdynamic $(shell llvm-config --cxxflags) -g -O0

%.so: %.o 
	$(CXX) -dylib -flat_namespace -shared $^ -o $@

live.o: live.cpp liveAnalysis.h IDFA.h

reach.o: reach.cpp reachAnalysis.h IDFA.h

clean:
	rm -f *.o *~ *.so
