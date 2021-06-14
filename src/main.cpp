#include "curler.h"
#include "logger.h"

#include <fstream>
#include <iostream>
#include <string>
#include <string.h>
#include <vector>

struct urldata
{
    std::string url;
    std::string filename;
    std::string path;
};

std::vector<urldata> parse_args(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    if (argc > 1 && strcmp(argv[1], "-h") == 0) {
	std::cout << "usage: curler [-h] [-p <path>] [-f <file>] [-u <url> [filename]]\n" << std::endl;
	std::cout << "arguments:\n\t-h\tShow this help message and exit\n"
		  << "\t-p\tPath to download into (defaults to current working directory if not specified)\n"
		  << "\t-f\tFilename to read urls and filenames from\n"
		  << "\t-u\tURL to download, followed by optional filename\n" << std::endl;
	std::cout << "example:\n\tcurler -p ~/Downloads -u https://example.com/file.mp4 video.mp4" << std::endl;
    } else if (argc > 1) {
	std::vector<urldata> urls = parse_args(argc, argv);
	bool res = false;

	for (const urldata &url : urls) {
	    if (url.filename.length() == 0)
		res = download(url.path, url.url);
	    else
		res = download(url.path, url.filename, url.url);
	    if (!res) log(err[FILE_ERR_DOWNLOAD], url.filename);
	}
	log(info[FILE_INFO_DONE]);

	return 0;

    } else {
	std::cout << "Usage: " << argv[0] << " [-p <path>] [-u] <url> [filename]" << std::endl;
	return -1;
    }
}

std::vector<urldata> parse_args(int argc, char *argv[])
{
    std::vector<urldata> urls;
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
	    data.path = path;
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
		    if (!(line.empty() or line.at(0) == ' ')) {
			space = line.find(' ');
			if (space != std::string::npos) {
			    data.url = line.substr(0, space);
			    data.filename = line.substr(space + 1);
			} else {
			    data.url = line;
			    data.filename = "";
			}
			data.path = path;
			urls.push_back(data);
		    }
		}
		continue;

	    } else {
		log(err[URL_ERR_TEXTFILE], argv[i]);
		exit(-1);
	    }

	// No flags passed. Try to parse following args as a url
	} else {
	    std::string temp_url = argv[i];
	    std::string temp_filename;
	    if (i+1 < argc)
		temp_filename = argv[++i];

	    if (temp_url.substr(0, 7).compare("http://") == 0
		|| temp_url.substr(0, 8).compare("https://") == 0
		|| temp_url.substr(0, 6).compare("ftp://") == 0) {
		data.url = temp_url;
		if (temp_filename.length() > 0)
		    data.filename = temp_filename;
	    } else {
		data.url = "";
		data.filename = "";
	    }
	    data.path = path;
	}

	urls.push_back(data);
    }

    return urls;
}
