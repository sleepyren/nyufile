CC = gcc
CFLAGS = -g
LIBS = -lcrypto

SRC = nyufile.c procedure.c
OBJ = $(SRC:.c=.o)
EXECUTABLE = nyufile

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(EXECUTABLE) $(OBJ)

