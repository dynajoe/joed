CC=g++
CFLAGS=-g -Wall

joedmake: joed.cpp v8/v8.o libuv/uv.a 
	$(CC) joed.cpp HttpWrap.h HttpWrap.cpp -lz -o joed -Iv8/include libuv/uv.a v8/out/x64.release/libv8_{base,snapshot}.a -lpthread -framework CoreServices $(CFLAGS)

libuv/uv.a:
	make -C libuv

v8/v8.o:
	make x64 -C v8
	
clean:
	rm -rf *.o joed
