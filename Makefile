downloader: main.cpp curler.cpp
	g++ -std=c++17 -Wall -o curler main.cpp curler.cpp -lcurl

.PHONY: clean

clean:
	rm -f curler
