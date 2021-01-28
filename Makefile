curler: main.cpp curler.cpp
	g++ -std=c++17 -Wall -O2 -lcurl -o curler main.cpp curler.cpp

.PHONY: debug
debug:
	g++ -g -std=c++17 -Wall -lcurl -o curler main.cpp curler.cpp

.PHONY: clean
clean:
	rm -f curler
