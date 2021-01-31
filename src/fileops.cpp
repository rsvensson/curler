#include "fileops.h"

#include <ctime>
#include <filesystem>
#include <string>
#include <sys/stat.h>
#include <utime.h>

void create_dir_if_not_exists(const std::string path) {
    if (!std::filesystem::is_directory(path) || std::filesystem::exists(path))
	std::filesystem::create_directory(path);
}

bool file_exists(const std::string &filename) {
    struct stat buffer;
    return (stat (filename.c_str(), &buffer) == 0);
}


long get_filesize(const std::string &filename) {
    struct stat buffer;
    int rc = stat(filename.c_str(), &buffer);
    return (rc == 0) ? buffer.st_size : -1;
}


/* Sets the file modification time to filetime */
bool set_filetime(const char *filename, const time_t filetime)
{
    struct stat buffer;
    struct utimbuf new_time;

    if (stat(filename, &buffer) < 0)
	return false;

    new_time.actime = buffer.st_atime;
    new_time.modtime = filetime;
    if (utime(filename, &new_time) < 0)
	return false;

    return true;
}


/* Remove any illegal characters from filename */
std::string clean_filename(const std::string &filename)
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
