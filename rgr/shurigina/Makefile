C_OPTIONS = -O0 -DWITH_MPI -lstdc++
TARGET		= main

all: $(TARGET) clean

clean:
	rm -rf *.o

$(TARGET): $(TARGET).o
	mpic++ $(C_OPTIONS) -o $(TARGET) $(TARGET).o

$(TARGET).o: $(TARGET).cpp 
	mpic++ $(C_OPTIONS) -c $(TARGET).cpp