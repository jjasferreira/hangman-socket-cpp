# The compiler: g++ for C++ program
  CC = g++

# Compiler flags:
  # -g   : adds debugging info to the executable file
  # -Wall: turns on most warnings
  CFLAGS = -g -Wall

# The build target 
  TARGET = gs player

all: $(TARGET)
 
gs: gs.cpp
	$(CC) $(CFLAGS) -o gs gs.cpp

player: player.cpp
	$(CC) $(CFLAGS) -o player player.cpp
 
clean:
	$(RM) $(TARGET)