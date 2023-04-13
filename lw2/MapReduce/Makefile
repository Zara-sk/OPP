C_OPTIONS = -O0 -g -w
TARGET		= main

all: $(TARGET) clean

clean:
	rm -rf *.o

$(TARGET): $(TARGET).o
	g++ $(C_OPTIONS) -o $(TARGET) $(TARGET).o

$(TARGET).o: $(TARGET).cpp $(TARGET).h
	g++ $(C_OPTIONS) -c $(TARGET).cpp