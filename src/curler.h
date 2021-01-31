#ifndef CURLER_H
#define CURLER_H

#include <string>

/* Tries to determine the filename automatically from the headers/url */
bool download(const std::string &path, const std::string &url);
/* Downloads file from the url and saves it as 'filename' */
bool download(const std::string &path, const std::string &filename, const std::string &url);

#endif
