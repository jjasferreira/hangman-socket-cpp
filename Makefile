# The compiler: g++ for C++ program
  CC = g++

# Compiler flags:
  # -g   : adds debugging info to the executable file
  # -Wall: turns on most warnings
  CFLAGS = -g -Wall

# The build target 
  TARGET = aux.o gs player
  OBJ = aux.o

all: $(TARGET)

aux.o: aux.cpp
	$(CC) -c $(CFLAGS) -o aux.o aux.cpp
 
gs: $(OBJ) server/gs.cpp
	$(CC) $(CFLAGS) -o gs server/gs.cpp $(OBJ)

player: $(OBJ) player.cpp
	$(CC) $(CFLAGS) -o player player.cpp $(OBJ)
 
clean:
	$(RM) $(TARGET) core