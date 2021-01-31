#ifndef FILEOPS_H
#define FILEOPS_H

#include <string>
#include <sys/stat.h>
#include <utime.h>

bool file_exists(const std::string &filename);
long get_filesize(const std::string &filename);
/* Sets the file modification time to filetime */
bool set_filetime(const char *filename, const time_t filetime);
/* Remove any illegal characters from filename */
std::string clean_filename(const std::string &filename);

#endif
