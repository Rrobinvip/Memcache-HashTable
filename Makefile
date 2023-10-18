CC = gcc
CFLAGS = -Wall 
TARGET = memcache
DEP1 = socket_utils
DEP2 = shared_hashtable
LIBS = -pthread
DDEBUG = -DDEBUG

all: $(TARGET)

$(TARGET): $(TARGET).o $(DEP1).o $(DEP2).o
	$(CC) $(DDEBUG) $(CFLAGS) $(LIBS) $(DEP1).o $(DEP2).o -o $(TARGET) $(TARGET).o

$(TARGET).o: $(TARGET).c
	$(CC) $(CFLAGS) $(LIBS) -c $(TARGET).c

$(DEP1).o: $(DEP1).c
	$(CC) $(CFLAGS) -c $(DEP1).c

$(DEP2).o: $(DEP2).c
	$(CC) $(DDEBUG) $(CFLAGS) $(LIBS) -c $(DEP2).c

clean:
	rm $(TARGET)
	rm *.o
