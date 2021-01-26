#include "curler.h"
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

struct urldata
{
    std::string url;
    std::string filename;
};

typedef std::map<std::string, std::vector<urldata> > URLMAP;

URLMAP parse_args(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    if (argc > 1) {
	URLMAP urls = parse_args(argc, argv);

	for (const auto &path :urls) {
	    for (const auto &url: path.second)
		if (url.filename.length() == 0) {
		    if (!download(path.first, url.url))
			std::cout << "Couldn't get file. Skipping." << std::endl;
		    else
			std::cout << "Done\n" << std::endl;

		} else {
		    if (!download(path.first, url.filename, url.url))
			std::cout << "Couldn't get '" << url.filename << "'. Skipping." << std::endl;
		    else
			std::cout << "Done\n" << std::endl;
		}
	}
	return 0;

    } else {
	std::cout << "Usage: " << argv[0] << " -p <path> -u <url> <filename>" << std::endl;
	return -1;
    }
}

URLMAP parse_args(int argc, char *argv[])
{
    URLMAP urls;
    urldata data;
    std::string path = "."; // Setting to . makes it possible to omit the -p flag
    std::string f = "-f";   // Flag for txt file containing urls
    std::string p = "-p";   // Flag for path
    std::string u = "-u";   // Flag for url

    for (int i=1; i < argc; i++) {
	if (p.compare(argv[i]) == 0) {
	    path = argv[++i];
	    continue;

	// Parse the urls, and handle having no filename
	} else if (u.compare(argv[i]) == 0) {
	    data.url = argv[++i];
	    if (i+1 < argc && u.compare(argv[i+1]) && p.compare(argv[i+1]))
		data.filename = argv[++i];
	    else
		data.filename = "";

	// Parse url/filename from textfile
	} else if (f.compare(argv[i]) == 0) {
	    std::ifstream fd;
	    fd.open(argv[++i], std::ios::in);

	    if (fd.is_open()) {
		std::string line;
		size_t space;
		while (std::getline(fd, line)) {
		    space = line.find(' ');
		    if (space != std::string::npos) {
			data.url = line.substr(0, space - 1);
			data.filename = line.substr(space + 1, line.back());
		    } else {
			data.url = line;
			data.filename = "";
		    }
		}

	    } else
		std::cout << "Couldn't open file." << std::endl;

	// No flags passed. Try to parse following args as a url
	} else {
	    std::string temp_url = argv[i];
	    std::string temp_filename;
	    if (i+1 < argc)
		temp_filename = argv[++i];

	    if (temp_url.substr(0, 7).compare("http://") == 0
		|| temp_url.substr(0, 6).compare("ftp://") == 0) {
		data.url = temp_url;
		if (temp_filename.length() > 0)
		    data.filename = temp_filename;
	    } else {
		data.url = "";
		data.filename = "";
	    }
	}

	urls[path].push_back(data);
    }

    return urls;
}
