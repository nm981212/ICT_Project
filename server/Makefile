CC:=gcc

TARGET_SRV=iot_server
OBJECT_SRV=$(TARGET_SRV).o 

LDFLAGS=-D_REENTRANT -pthread

$(TARGET_SRV):$(OBJECT_SRV)
	$(CC) -o $@ $(OBJECT_SRV) $(LDFLAGS)

%.o:%.c
	$(CC) -c -o $@ $<
clean:
	rm -f *.o $(TARGET_SRV)
