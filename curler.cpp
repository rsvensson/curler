#include "curler.h"

#include <curl/curl.h>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <map>
#include <string.h>
#include <sys/stat.h>
#include <variant>

#define KBYTE double(1024)
#define MBYTE (double(1024) * KBYTE)
#define GBYTE (double(1024) * MBYTE)
#define TBYTE (double(1024) * GBYTE)

typedef std::map<const std::string, std::variant<long, std::string>>  HEADERS;
typedef std::map<const std::string, const std::string> MIMETYPES;

// Function prototypes
static bool file_exists(const std::string &filename);
static long get_filesize(const std::string &filename);
static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream);
static int progress_func(void *ptr, double total_to_download, double now_downloaded,
			 double total_to_upload, double now_uploaded);
static size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata);
static HEADERS get_headers(const std::string &url);
static std::string clean_filename(const std::string &filename);
static bool do_download(const char *filename, const char *url, curl_off_t resume_point);
bool download(const std::string &path, const std::string &url);
bool download(const std::string &path, const std::string &filename, const std::string &url);


static MIMETYPES mimetypes = {
    { "application/pdf",                   ".pdf" },
    { "application/pgp-signature",         ".sig" },
    { "application/futuresplash",          ".spl" },
    { "application/octet-stream",          ".class" },
    { "application/postscript",            ".ps" },
    { "application/x-bittorrent",          ".torrent" },
    { "application/x-dvi",                 ".dvi" },
    { "application/x-gzip",                ".gz" },
    { "application/x-tar",                 ".tar" },
    { "application/x-tgz",                 ".tar.gz" },
    { "application/x-bzip",                ".bz2" },
    { "application/x-bzip-compressed-tar", ".tar.bz2" },
    { "application/zip",                   ".zip" },
    { "application/x-ns-proxy-autoconfig", ".pac" },
    { "application/x-shockwave-flash",     ".swf" },
    { "application/ogg",                   ".ogg" },
    { "audio/mpeg",                        ".mp3" },
    { "audio/x-mpegurl",                   ".m3u" },
    { "audio/x-ms-wma",                    ".wma" },
    { "audio/x-ms-wax",                    ".wax" },
    { "audio/x-wav",                       ".wav" },
    { "audio/mp4",                         ".m4a" },
    { "audio/flac",                        ".flac" },
    { "image/gif",                         ".gif" },
    { "image/jpg",                         ".jpg" },
    { "image/png",                         ".png" },
    { "image/svg+xml",                     ".svg" },
    { "image/x-xbitmap",                   ".xbm" },
    { "image/x-xpixmap",                   ".xpm" },
    { "image/x-xwindowdump",               ".xwd" },
    { "text/css",                          ".css" },
    { "text/html",                         ".html" },
    { "text/javascript",                   ".js" },
    { "text/plain",                        ".txt" },
    { "text/xml",                          ".xml" },
    { "video/mpeg",                        ".avi" },
    { "video/quicktime",                   ".mov" },
    { "video/x-flv",                       ".flv" },
    { "video/x-ms-asf",                    ".asf" },
    { "video/x-ms-wmv",                    ".wvv" },
    { "video/x-matroska",                  ".mkv" },
    { "video/mp4",                         ".mp4" },
    { "video/webm",                        ".webm" }
};

static bool file_exists(const std::string &filename) {
    struct stat buffer;
    return (stat (filename.c_str(), &buffer) == 0);
}


static long get_filesize(const std::string &filename) {
    struct stat buffer;
    int rc = stat(filename.c_str(), &buffer);
    return (rc == 0) ? buffer.st_size : -1;
}

// custom callback function for CURLOPT_WRITEFUNCTION
static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    std::size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

// custom callback function for CURLOPT_PROGRESSFUNCTION to create a progress bar
static int progress_func(void *ptr, double total_to_download, double now_downloaded,
			 double total_to_upload, double now_uploaded)
{
    // makes sure the file is not empty
    if (total_to_download <= 0.0) {
	return 0;
    }

    // determine what units we will use
    const char *dlunit;
    const char *ndunit;
    double total_size;
    double downloaded;
    if (total_to_download >= TBYTE) {
	dlunit = "TiB";
	total_size = total_to_download / TBYTE;
    } else if (total_to_download >= GBYTE) {
	dlunit = "GiB";
	total_size = total_to_download / GBYTE;
    } else if (total_to_download >= MBYTE) {
	dlunit = "MiB";
	total_size = total_to_download / MBYTE;
    } else if (total_to_download >= KBYTE) {
	dlunit = "KiB";
	total_size = total_to_download / KBYTE;
    } else {
	dlunit = "B";
	total_size = total_to_download;
    }

    if (now_downloaded >= TBYTE) {
	ndunit = "TiB";
	downloaded = now_downloaded / TBYTE;
    } else if (now_downloaded >= GBYTE) {
	ndunit = "GiB";
	downloaded = now_downloaded / GBYTE;
    } else if (now_downloaded >= MBYTE) {
	ndunit = "MiB";
	downloaded = now_downloaded / MBYTE;
    } else if (now_downloaded >= KBYTE) {
	ndunit = "KiB";
	downloaded = now_downloaded / KBYTE;
    } else {
	ndunit = "B";
	downloaded = now_downloaded;
    }

    // width of progress bar
    int totaldots = 40;
    double fraction_downloaded = now_downloaded / total_to_download;
    // part of the progress bar that's already full
    int dots = static_cast<int>(round(fraction_downloaded * totaldots));

    // create the meter.
    // need to use cstdio tools because cout seems to produce
    // incorrect output?
    int i;
    printf("%3.0f%% [", fraction_downloaded*100);
    for (i=0; i < dots; i++) {
	printf("=");
    }
    for (; i < totaldots; i++) {
	printf(" ");
    }
    printf("] %3.0f %s / %3.0f %s\r", downloaded, ndunit, total_size, dlunit);
    fflush(stdout);
    std::cout.flush();

    // Must return 0 otherwise the transfer is aborted
    return 0;
}

// Custom callback function for CURLOPT_HEADERFUNCTION to extract header data
// Currently only used to get the filename if available.
static size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
    char *cd = (char *)userdata;

    // TODO: this leaves a trailing " character for some reason even though it's escaped
    sscanf(buffer, "content-disposition: %*s %*s filename=\"%s\"", cd);

    return nitems * size;
}


// Get headers so we can determine size and mimetype of content
static HEADERS get_headers(const std::string &url)
{
    CURL *curl;
    HEADERS headers;
    double content_length = 0.0;
    char *content_type = nullptr;

    curl = curl_easy_init();
    if (curl) {
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_perform(curl);
	curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &content_length);
	curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type);

    }

    headers["length"] = static_cast<long>(content_length);
    headers["filetype"] = mimetypes[content_type];

    curl_easy_cleanup(curl);

    return headers;
}


// Remove any illegal characters from filename
static std::string clean_filename(const std::string &filename)
{
    std::string clean_name;

    for (size_t i=0; i < filename.length(); i++) {
	if (filename[i] == '/' || filename[i] == '\\' || filename[i] == '*' || filename[i] == '&'
	    || filename[i] == ':' || filename[i] == '<' || filename[i] == '>')
	{
	    // It often looks pretty decent to change ':' to " -"
	    // Like in "Title: Subtitle" -> "Title - Subtitle"
	    if (filename[i] == ':') {
		clean_name += " -";
	    } else {
		// Replace bad character with a space
		clean_name += ' ';
	    }
	}
	else
	    clean_name += filename[i];
    }

    return clean_name;
}


// The actual download function
static bool do_download(const char *filename, const char *url, curl_off_t resume_point)
{
    CURL *curl;
    FILE *fp;

    curl = curl_easy_init();
    if (curl) {
	if (resume_point != 0)
	    fp = fopen(filename, "a+b");
	else
	    fp = fopen(filename, "wb");
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, resume_point);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_func);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

	std::cout << "Downloading to " << filename << std::endl;
	curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	std::fclose(fp);
	return true;
    } else
	return false;
}


// Determine filename from url
bool download(const std::string &path, const std::string &url)
{
    CURL *curl;
    char content_disposition[1000] = "None";
    std::string filename;

    curl = curl_easy_init();
    if (curl) {
	// First try to get filename through content-disposition header
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, &content_disposition);
	curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	if (strcmp(content_disposition, "None") != 0) {
	    // Make sure we have a nice decoded filename if we found one
	    int outlength;
	    filename = curl_easy_unescape(curl, content_disposition,
					  strlen(content_disposition), &outlength);
	    // Temp workaround for removing the trailing "
	    filename.pop_back();
	}
	// Filename not found in content-disposition. Try the url.
	else {
	    char sep = '/';
	    char php = '?';  // To hopefully remove any pesky trailing php arguments
	    size_t s = url.rfind(sep, url.length());

	    if (s != std::string::npos) {
		filename = url.substr(s+1, url.length() - s);
		size_t p = filename.rfind(php, filename.length());
		if (p != std::string::npos)
		    filename = filename.substr(0, p-1);
	    }
	    // Last resort
	    else {
		std::cout << "Couldn't determine filename." << std::endl;
		std::string name = "file";
		int number = 1;
		do {
		    filename = name + std::to_string(number);
		    number++;
		} while (file_exists(filename));
	    }
	}

	return download(path, filename, url);
    } else
	return false;
}


// Prepares the filename and checks if we've downloaded the file already.
// The actual download happens in do_download()
bool download(const std::string &path, const std::string &filename, const std::string &url)
{
    HEADERS headers = get_headers(url.c_str());
    long content_length = std::get<long>(headers["length"]);
    std::string filetype = std::get<std::string>(headers["filetype"]);
    std::string fullpath;
    std::string clean_fname = clean_filename(filename);

    if (path.back() != '/')
	fullpath = path + '/' + clean_fname;

    // Make sure we have the file ending
    if (fullpath.substr(fullpath.length() - filetype.length()) != filetype)
	fullpath.append(filetype);

    bool is_ftp = (url.substr(0, 6).compare("ftp://") == 0);
    bool is_downloaded = file_exists(fullpath);

    if (is_downloaded) {
	long filesize = get_filesize(fullpath);
	if (content_length == filesize) {
	    std::cout << "File '" << clean_fname << "' already downloaded. Skipping." << std::endl;
	    return true;
	}

	std::cout << "Found incomplete file at '" << fullpath << "'." << std::endl;
	std::cout << "Resuming at " << filesize << "." << std::endl;
	if (is_ftp)  // For ftp it's recommended to resume from -1
	    return do_download(fullpath.c_str(), url.c_str(), -1);
	else
	    return do_download(fullpath.c_str(), url.c_str(), filesize);
    } else
	return do_download(fullpath.c_str(), url.c_str(), 0);
}
