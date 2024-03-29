#ifndef FILEOPS_H
#define FILEOPS_H

#include <string>

namespace fileops {
    void create_dir_if_not_exists(const std::string path);
    bool is_writeable(const std::string path);
    bool file_exists(const std::string &filename);
    std::uintmax_t get_filesize(const std::string &filename);
    /* Sets the file modification time to filetime */
    bool set_filetime(const std::string &filename, const time_t filetime);
    /* Gets the file modification time from filename */
    time_t get_filetime(const std::string &filename);
    /* Remove any illegal characters from filename */
    std::string clean_filename(const std::string &filename);
}

#endif
