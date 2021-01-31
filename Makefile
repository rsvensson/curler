curler: src/main.cpp src/curler.cpp src/fileops.cpp src/callbacks.cpp
	g++ -std=c++17 -Wall -O2 -lcurl -o curler src/*.cpp

.PHONY: debug
debug:
	g++ -g -std=c++17 -Wall -lcurl -o curler src/*.cpp

.PHONY: clean
clean:
	rm -f curler
