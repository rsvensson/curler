#include "logger.h"
#include <iostream>
#include <string>

std::string err[] = {"Path is not writeable"};

std::string warn[] = {
    "Tried but couldn't set file modification time to remote file time",
    "Couldn't determine filename"
};

std::string info[] = {
    "Found incomplete file at",
    "Skipping already downloaded file:",
    "Resuming download at byte",
    "Downloading to"
};


void log(std::string msg) {
    std::cout << msg << std::endl;
}

void log(std::string msg, std::string ext) {
    std::cout << msg << " " << ext << "." << std::endl;
}

void log(std::string msg, long ext) {
    std::cout << msg << " " << ext << "." << std::endl;
}
