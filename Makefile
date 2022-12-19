# The compiler: g++ for C++ program
  CC = g++

# Compiler flags:
  # -g   : adds debugging info to the executable file
  # -Wall: turns on most warnings
  CFLAGS = -g -Wall

# The build target 
  TARGET = common.o gs player
  OBJ = common.o

all: $(TARGET)

common.o: common.cpp
	$(CC) -c $(CFLAGS) -o common.o common.cpp
 
gs: $(OBJ) server/gs.cpp
	$(CC) $(CFLAGS) -o gs server/gs.cpp $(OBJ)

player: $(OBJ) player.cpp
	$(CC) $(CFLAGS) -o player player.cpp $(OBJ)
 
clean:
	$(RM) $(TARGET) core