curler: src/main.cpp src/curler.cpp src/fileops.cpp src/logger.cpp src/callbacks.c
	g++ -std=c++17 -Wall -O2 -lcurl -o curler src/*.cpp src/*.c

.PHONY: debug
debug:
	g++ -g -std=c++17 -Wall -lcurl -o curler src/*.cpp src/*.c

.PHONY: clean
clean:
	rm -f curler
