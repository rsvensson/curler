#ifndef LOGGER_H
#define LOGGER_H

#include <string>

enum {
    FILE_ERR_PERMS,
    FILE_ERR_DOWNLOAD,
    FILE_ERR_FILETIME,
    URL_ERR_TEXTFILE,
    URL_ERR_EMPTY
};

enum {
    FILE_WARN_FILETIME,
    FILE_WARN_FILENAME,
    FILE_WARN_FILETYPE,
    FILE_WARN_FILESIZE
};

enum {
    FILE_INFO_EXISTS,
    FILE_INFO_SKIP,
    FILE_INFO_RESUME,
    FILE_INFO_DOWNLOAD,
    FILE_INFO_DONE,
    DEBUG_INFO_OUT
};

extern std::string err[];
extern std::string warn[];
extern std::string info[];

void log(std::string msg);
void log(std::string msg, std::string ext);
void log(std::string msg, long ext);

#endif
