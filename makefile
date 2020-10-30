make:
	gcc Server\ Offline\ Messenger.c -lpthread -o Server\ Offline\ Messenger
	gcc Client\ Offline\ Messenger.c -o Client\ Offline\ Messenger
clean:
	rm Server\ Offline\ Messenger
	rm Client\ Offline\ Messenger
