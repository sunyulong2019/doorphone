SRC = $(wildcard *.c)
ELF = doorphone 

CPPFLAGS += -Wall
CPPFLAGS += -DDEBUG
CPPFLAGS += -Iinc/

LDFLAGS += -L../lib/
LDFLAGS += -lasound
LDFLAGS += -lpthread
LDFLAGS += -ljpeg
LDFLAGS += -lts

LDFLAGS += -Wl,-rpath=.
LDFLAGS += -Wl,-rpath=./lib

CROSS_COMPILE = arm-none-linux-gnueabi-
CC = $(CROSS_COMPILE)gcc

$(ELF):$(SRC)
	$(CC) $^ -o $@ $(CPPFLAGS) $(LDFLAGS)

clean:
	-@$(RM) $(ELF)
