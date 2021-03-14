#include "curler.h"
#include "callbacks.h"
#include "fileops.h"
#include "logger.h"

#include <curl/curl.h>
#include <map>
#include <string.h>
#include <variant>

using HEADERS = std::map<const std::string, std::variant<long, std::string>>;
using MIMETYPES = std::map<const std::string, const std::string>;

/* Function prototypes */
static HEADERS get_headers(const std::string &url);
static bool do_download(const char *filename, const char *url, const curl_off_t resume_point);
bool download(const std::string &path, const std::string &url);
bool download(const std::string &path, const std::string &filename, const std::string &url);


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


/* Get headers so we can determine size and mimetype of content */
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
    if (content_type)
	headers["filetype"] = mimetypes[content_type];
    else {  // Didn't find content-type. Try to get extension from the url.
	std::string ct = url.substr(url.rfind('.'), url.back());
	if (ct.length() > 0)
	    headers["filetype"] = ct;
    }

    curl_easy_cleanup(curl);

    return headers;
}


/* The actual download function */
static bool do_download(const char *filename, const char *url, const curl_off_t resume_point = 0)
{
    CURL *curl;
    CURLcode res;
    FILE *fp;
    time_t filetime;

    curl = curl_easy_init();
    if (curl) {
	if (resume_point != 0)
	    fp = fopen(filename, "a+b");
	else
	    fp = fopen(filename, "wb");
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_FILETIME, 1L);
	curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, resume_point);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

	log(info[FILE_INFO_DOWNLOAD], filename);
	res = curl_easy_perform(curl);
	std::fclose(fp);

	// Try to set file modification time to remote file time
	if (CURLE_OK == res) {
	    res = curl_easy_getinfo(curl, CURLINFO_FILETIME, &filetime);
	    if ((CURLE_OK == res) && (filetime >= 0)) {
		if (!set_filetime(filename, filetime))
		    log(warn[FILE_WARN_FILETIME]);
		    //printf("\nTried but couldn't set file modification time to remote file time.\n");
	    }
	}

	curl_easy_cleanup(curl);
	return true;
    } else
	return false;
}


/* Determine filename, then send to the other download function */
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
		log(warn[FILE_WARN_FILENAME]);
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


/*
 * Prepares the filename and checks if we've downloaded the file already.
 * The actual download happens in do_download()
 */
bool download(const std::string &path, const std::string &filename, const std::string &url)
{
    /* Check that we have write permissions */
    if (!is_writeable(path)) {
	log(err[FILE_ERR_PERMS]);
	return false;
    }

    HEADERS headers = get_headers(url.c_str());
    long content_length = std::get<long>(headers["length"]);
    std::string filetype = std::get<std::string>(headers["filetype"]);
    std::string fullpath;
    std::string clean_fname = clean_filename(filename);

    if (path.back() != '/')
	fullpath = path + '/' + clean_fname;
    else
	fullpath = path + clean_fname;

    // Make sure we have the file ending
    if (fullpath.substr(fullpath.length() - filetype.length()) != filetype)
	fullpath.append(filetype);

    if (file_exists(fullpath)) {
	long filesize = get_filesize(fullpath);
	if (content_length == filesize) {
	    log(info[FILE_INFO_SKIP], clean_fname);
	    return true;
	}

	log(info[FILE_INFO_EXISTS], fullpath);
	log(info[FILE_INFO_RESUME], filesize);
	return do_download(fullpath.c_str(), url.c_str(), filesize);
    } else {
	create_dir_if_not_exists(path);
	return do_download(fullpath.c_str(), url.c_str());
    }
}
