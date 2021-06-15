#ifndef CURLER_H
#define CURLER_H

#include <string>

bool download(const std::string &url, const std::string &path,
	      const std::string &filename);

#endif
