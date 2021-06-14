#include "logger.h"
#include <iostream>
#include <string>

std::string err[] = {
    "Path is not writeable",
    "Couldn't download file",
    "Tried but couldn't set file modification time to remote file time",
    "Couldn't open"
};

std::string warn[] = {
    "Couldn't determine file modification time",
    "Couldn't determine filename",
    "Couldn't determine filetype",
    "Remote and local file modification time match, but size is different.\nRedownloading"
};

std::string info[] = {
    "Found incomplete file at",
    "Skipping already downloaded file:",
    "Resuming download at byte",
    "Downloading to",
    "Done",
    "DEBUG:"
};


void log(std::string msg) {
    std::cout << msg << std::endl;
}

void log(std::string msg, std::string ext) {
    std::cout << msg << " \"" << ext << "\"." << std::endl;
}

void log(std::string msg, long ext) {
    std::cout << msg << " " << ext << "." << std::endl;
}
