PRO_DIR := .
STATE_C := $(PRO_DIR)/state.c
ROUTE_C := $(PRO_DIR)/route.c

CC = gcc
CFLAGS = -I .


main: main.o state.o route.o
	$(CC) -o $@ $^ $(CFLAGS)

main.o: main.c
	$(CC) -c $< $(CFLAGS)

route.o: $(ROUTE_C)
	$(CC) -c $< $(CFLAGS)

state.o: $(STATE_C)
	$(CC) -c $< $(CFLAGS)

clean:
	@rm main main.o route.o state.o
