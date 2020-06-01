all:
	$(CC) -pthread user_mod.c -o user
	$(CC) -pthread central_server.c -o server
clean:
	rm user
	rm server
