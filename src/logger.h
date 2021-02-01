#ifndef LOGGER_H
#define LOGGER_H

#include <string>

enum { FILE_ERR_PERMS };
enum { FILE_WARN_FILETIME, FILE_WARN_FILENAME };
enum {
    FILE_INFO_EXISTS,
    FILE_INFO_SKIP,
    FILE_INFO_RESUME,
    FILE_INFO_DOWNLOAD
};

extern std::string err[];
extern std::string warn[];
extern std::string info[];

void log(std::string msg);
void log(std::string msg, std::string ext);
void log(std::string msg, long ext);

#endif
