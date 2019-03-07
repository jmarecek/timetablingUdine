SYSTEM = x86_debian4.0_4.1
LIBFORMAT = static_pic
CCC = g++

# System/lib-format specific compile/link flags
CSYSFLAGS  = -fPIC -DIL_STD 

# Compilation and linking flags enabling Multi-threading
CMTFLAGS = -DILOUSEMT -D_REENTRANT
LDMTFLAGS = -lpthread

# Code optimization/debugging options
DEBUG = -O3 -DNDEBUG

CONCERTDIR=/home/jxm/concert29
CPLEXDIR=/home/jxm/cplex121

CFLAGS = $(CSYSFLAGS) $(DEBUG) -I$(CONCERTDIR)/include/ -I$(CPLEXDIR)/include/ -I./src/ -I./src/cliquer/ -I/home/jxm/udine-bc/boost/ $(OPTIONS)  

LDFLAGS = -L$(CPLEXDIR)/lib/$(SYSTEM)/$(LIBFORMAT) -lilocplex -lcplex -L$(CONCERTDIR)/lib/$(SYSTEM)/$(LIBFORMAT) -lconcert -ldl -lpthread

#---------------------------------------------------------
# FILES
#---------------------------------------------------------

TARGET = ./bin/udine 

execute: $(TARGET)
	./bin/udine ./data/comp01.ctt

build: $(TARGET)

clean:
	/bin/rm -rf *.o
	/bin/rm -rf $(TARGET)
	touch out.out
	/bin/rm -rf *.out

./bin/reorder.o:
	$(CCC) $(CFLAGS) -o ./bin/reorder.o ./src/cliquer/reorder.cpp -c
./bin/graph.o:
	$(CCC) $(CFLAGS) -o ./bin/graph.o ./src/cliquer/graph.cpp -c
./bin/cliquer.o:
	$(CCC) $(CFLAGS) -o ./bin/cliquer.o ./src/cliquer/cliquer.cpp -c
./bin/loader.o: ./src/loader.cpp
	$(CCC) $(CFLAGS) -o ./bin/loader.o ./src/loader.cpp -c
./bin/solver.o: ./src/solver.cpp
	$(CCC) $(CFLAGS) -o ./bin/solver.o ./src/solver.cpp -c
./bin/conflicts.o: ./src/conflicts.cpp
	$(CCC) $(CFLAGS) -o ./bin/conflicts.o ./src/conflicts.cpp -c
./bin/cut_manager.o: ./src/cut_manager.cpp
	$(CCC) $(CFLAGS) -o ./bin/cut_manager.o ./src/cut_manager.cpp -c
./bin/test.o: ./src/test/test.cpp
	$(CCC) $(CFLAGS) -o ./bin/test.o ./src/test/test.cpp -c
./bin/udine: ./bin/conflicts.o ./bin/cut_manager.o ./bin/loader.o ./bin/solver.o ./bin/test.o ./bin/reorder.o ./bin/graph.o ./bin/cliquer.o
	$(CCC) -o ./bin/udine ./bin/reorder.o ./bin/graph.o ./bin/cliquer.o ./bin/conflicts.o ./bin/cut_manager.o ./bin/loader.o ./bin/solver.o ./bin/test.o $(LDFLAGS) 
