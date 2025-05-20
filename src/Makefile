CFLAGS = -g -Wall -Wextra -Werror -fsanitize=address,leak,object-size,bounds-strict,undefined -fsanitize-address-use-after-scope

all: ash chat_server chat_client send

ash: ash.o builtins.o io_helpers.o linked_list.o
	gcc ${CFLAGS} -o $@ $^

chat_server: chat_server.o socket.o chat_helpers.o
	gcc ${CFLAGS} -o $@ $^

chat_client: chat_client.o socket.o
	gcc ${CFLAGS} -o $@ $^

send: send.o socket.o
	gcc ${CFLAGS} -o $@ $^

%.o: %.c builtins.h io_helpers.h linked_list.h
	gcc ${CFLAGS} -c $< 

clean:
	rm *.o ash chat_server chat_client send
