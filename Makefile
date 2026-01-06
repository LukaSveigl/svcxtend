CC = cc
CFLAGS = -Wall -Werror -pedantic

MAIN = svcxtend

all:
	$(CC) $(CFLAGS) $(MAIN).c -o $(MAIN)

clean:
	rm -f $(MAIN) vgcore.*
