#include "fileops.h"

#include <ctime>
#include <filesystem>
#include <string>
#include <sys/stat.h>
#include <utime.h>

namespace fs = std::filesystem;

using namespace fileops;

void fileops::create_dir_if_not_exists(const std::string path) {
    if (!fs::is_directory(path) || !fs::exists(path))
	fs::create_directory(path);
}

bool fileops::is_writeable(const std::string path) {
    fs::perms p = fs::status(path).permissions();
    if ((p & fs::perms::owner_write) != fs::perms::none)
	return true;
    return false;
}

bool fileops::file_exists(const std::string &filename) {
    return fs::exists(filename);
}


std::uintmax_t fileops::get_filesize(const std::string &filename) {
    return fs::file_size(filename);
}


/* Sets the file modification time to filetime */
bool fileops::set_filetime(const std::string &filename, const time_t filetime)
{
    struct stat buffer;
    struct utimbuf new_time;

    if (stat(filename.c_str(), &buffer) < 0)
	return false;

    new_time.actime = buffer.st_atime;
    new_time.modtime = filetime;
    if (utime(filename.c_str(), &new_time) < 0)
	return false;

    return true;
}


/* Gets the file modification time from filename */
time_t fileops::get_filetime(const std::string &filename)
{
    struct stat buffer;
    time_t filetime = 0;

    if (stat(filename.c_str(), &buffer) == 0)
	filetime = buffer.st_mtime;

    return filetime;
}


/* Remove any illegal characters from filename */
std::string fileops::clean_filename(const std::string &filename)
{
    std::string clean_name;

    for (size_t i=0; i < filename.length(); i++) {
	if (filename[i] == '/' || filename[i] == '\\' || filename[i] == '*' || filename[i] == '&'
	    || filename[i] == ':' || filename[i] == '<' || filename[i] == '>')
	{
	    /* It often looks pretty decent to change ':' to " -"
	     * Like in "Title: Subtitle" -> "Title - Subtitle" */
	    if (filename[i] == ':')
		clean_name += " -";
	    else
		// Replace bad character with a space
		clean_name += ' ';
	}
	else
	    clean_name += filename[i];
    }

    return clean_name;
}
