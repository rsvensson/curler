curler: src/main.cpp src/curler.cpp
	g++ -std=c++17 -Wall -O2 -lcurl -o curler src/main.cpp src/curler.cpp

.PHONY: debug
debug:
	g++ -g -std=c++17 -Wall -lcurl -o curler src/main.cpp src/curler.cpp

.PHONY: clean
clean:
	rm -f curler
