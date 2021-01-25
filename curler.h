#ifndef CURLER_H
#define CURLER_H

#include <string>

bool download(const std::string &path, const std::string &url);
bool download(const std::string &path, const std::string &filename, const std::string &url);

#endif
