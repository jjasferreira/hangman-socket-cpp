# The compiler: g++ for C++ program
  CC = g++

# Compiler flags:
  # -g   : adds debugging info to the executable file
  # -Wall: turns on most warnings
  CFLAGS = -g -Wall

# The build target 
  TARGET = program

all: $(TARGET)
 
$(TARGET): $(TARGET).cpp
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).cpp
 
clean:
	$(RM) $(TARGET)