#include "curler.h"
#include "callbacks.h"
#include "fileops.h"
#include "logger.h"

#include <curl/curl.h>
#include <map>
#include <string.h>

struct headers {
    long content_length = 0;
    char content_type[16];
    char content_disposition[512] = "None";
    time_t filetime = 0;
};

using MIMETYPES = std::map<const std::string, const std::string>;


/* Function prototypes */
static headers get_headers(const std::string &url, CURL *curl);
static std::string find_filename(const std::string &url, const headers &hdrs,
				 CURL *curl);
static curl_off_t get_resume_point(const std::string &fullpath,
				   const headers &hdrs);
static std::string get_fullpath(const std::string &path,
				const std::string &filename,
				const std::string &filetype);
bool download(const std::string &url, const std::string &path,
	      const std::string &filename);


/* For getting the extension of files if not specified. */
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


static headers get_headers(const std::string &url, CURL *curl)
{
    headers hdrs;
    char *content_type = nullptr;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_FILETIME, 1L);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &hdrs.content_disposition);
    curl_easy_perform(curl);

    curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &hdrs.content_length);
    curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type);
    curl_easy_getinfo(curl, CURLINFO_FILETIME, &hdrs.filetime);

    if (content_type)
	strcpy(hdrs.content_type, mimetypes[content_type].c_str());
    else if (url.rfind('.') != std::string::npos)  // Didn't find content-type. Try to get extension from the url.
	strcpy(hdrs.content_type, url.substr( url.rfind('.'), url.back() ).c_str());
    else {
	// Can't determine file type. Set to .bin
	log(warn[FILE_WARN_FILETYPE]);
	strcpy(hdrs.content_type, ".bin");
    }

    curl_easy_reset(curl);

    return hdrs;
}


static std::string find_filename(const std::string &url, const headers &hdrs, CURL *curl)
{
    std::string filename;

    if (strcmp(hdrs.content_disposition, "None") != 0) {
	// Make sure we have a nice decoded filename if we found one
	int outlength;
	filename = curl_easy_unescape(curl, hdrs.content_disposition,
				      strlen(hdrs.content_disposition), &outlength);
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
		filename = filename.substr(0, p);
	}
	// Last resort
	else {
	    log(warn[FILE_WARN_FILENAME]);
	    std::string name = "file";
	    int number = 1;
	    do {
		filename = name + std::to_string(number);
		number++;
	    } while (fileops::file_exists(filename));
	}
    }

    return filename;
}


/*
 * Checks if a file has already been downloaded, and if so checks if it should
 * be skipped or if we should resume the download.
 * Returns the byte we should resume at, 0 if it should be redownloaded (i.e.
 * start from the beginning again), or -1 if it should be skipped.
 */
static curl_off_t get_resume_point(const std::string &fullpath, const headers &hdrs)
{
    std::string filename = fullpath.substr(fullpath.rfind('/'), fullpath.length());

    if (fileops::file_exists(fullpath)) {
	// Compare modification time, and fall back on filesize
	time_t local_filetime = fileops::get_filetime(fullpath);
	long local_filesize = fileops::get_filesize(fullpath);

	// Check if both filetime and filesize match
	if (hdrs.filetime > 0 && hdrs.filetime == local_filetime) {
	    if (hdrs.content_length != local_filesize) {
		// Redownload file
		log(warn[FILE_WARN_FILESIZE], fullpath);
		return 0;
	    }

	    log(info[FILE_INFO_SKIP], filename);
	    return -1;
	}

	// Couldn't determine filetime, only check filesize
	if (hdrs.filetime <= 0 && hdrs.content_length == local_filesize) {
	    log(info[FILE_INFO_SKIP], filename);
	    return -1;
	}

	log(info[FILE_INFO_EXISTS], fullpath);
	log(info[FILE_INFO_RESUME], local_filesize);
	return local_filesize;
    } else {
	return 0;
    }
}


static std::string get_fullpath(const std::string &path, const std::string &filename, const std::string &filetype)
{
    std::string fullpath;

    if (path.back() != '/')
	fullpath = path + '/' + filename;
    else
	fullpath = path + filename;

    // Make sure we have the file ending
    if (fullpath.substr(fullpath.length() - filetype.length()) != filetype)
	fullpath.append(filetype);

    return fullpath;
}


bool download(const std::string &url, const std::string &path, const std::string &filename)
{
    CURL *curl = curl_easy_init();

    if (curl) {
	curl_off_t resume_point;
	CURLcode res;
	FILE *fp;
	headers hdrs = get_headers(url, curl);
	std::string fname = filename;
	std::string fullpath;


	if (fname.empty())
	    fname = find_filename(url, hdrs, curl);
	fname = fileops::clean_filename(fname);
	fullpath = get_fullpath(path, fname, hdrs.content_type);
	resume_point = get_resume_point(fullpath, hdrs);

	/* Check that we have write permissions */
	if (!fileops::is_writeable(path)) {
	    log(err[FILE_ERR_PERMS]);
	    curl_easy_cleanup(curl);
	    return false;
	}

	if (resume_point != 0)
	    fp = std::fopen(fullpath.c_str(), "a+b");
	else
	    fp = std::fopen(fullpath.c_str(), "wb");
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, resume_point);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

	log(info[FILE_INFO_DOWNLOAD], fullpath);
	res = curl_easy_perform(curl);
	std::fclose(fp);

	// Try to set file modification time to remote file time
	if (CURLE_OK == res) {
	    res = curl_easy_getinfo(curl, CURLINFO_FILETIME, &hdrs.filetime);
	    if ((CURLE_OK == res) && (hdrs.filetime >= 0)) {
		if (!fileops::set_filetime(filename, hdrs.filetime))
		    log(err[FILE_ERR_FILETIME]);
	    } else
		log(warn[FILE_WARN_FILETIME]);
	}

	curl_easy_cleanup(curl);
	return true;
    } else
	return false;
}
