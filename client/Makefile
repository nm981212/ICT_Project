CC:=gcc

TARGET_CLN=client
OBJECT_CLN=$(TARGET_CLN).o  

LDFLAGS=-D_REENTRANT -pthread
LDFLAGS_SQL=-D_REENTRANT -pthread -lmysqlclient

all : $(TARGET_CLN)

$(TARGET_CLN):$(OBJECT_CLN)
	$(CC) -o $@ $(OBJECT_CLN) $(LDFLAGS_SQL)
%.o:%.c
	$(CC) -c -o $@ $<
clean:
	rm -f *.o 
