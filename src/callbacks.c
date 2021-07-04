#include "callbacks.h"

#include <math.h>
#include <stdio.h>
#include <time.h>

#define KBYTE  double(1024)
#define MBYTE (double(1024) * KBYTE)
#define GBYTE (double(1024) * MBYTE)
#define TBYTE (double(1024) * GBYTE)

#define MINUTE 60
#define HOUR  (60 * MINUTE)
#define DAY   (24 * HOUR)

/* custom callback function for CURLOPT_WRITEFUNCTION. */
size_t write_callback(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}


/*
 * Custom callback function for CURLOPT_HEADERFUNCTION to extract header data.
 * Currently only used to get the filename and location if available.
 */
size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
    txt_headers *hdrs = (txt_headers *)userdata;

    // TODO: this leaves a trailing " character for some reason even though it's escaped
    sscanf(buffer, "content-disposition: %*s %*s filename=\"%s\"", hdrs->content_disposition);

    sscanf(buffer, "location: %s", hdrs->location);

    return nitems * size;
}


/* custom callback function for CURLOPT_PROGRESSFUNCTION to create a progress bar. */
int progress_callback(void *ptr, double total_to_download, double now_downloaded,
			 double total_to_upload, double now_uploaded)
{
    /* Make sure the file is not empty */
    if (total_to_download <= 0.0)
	return 0;

    /* Measure download speed & time remaining */
    long *resume_point = (long*)ptr; // Byte we resume the download at, if file exists. Otherwise 0.
    double actual_dl_size = total_to_download + (double)*resume_point;
    double actual_now_dl = now_downloaded + (double)*resume_point;
    static double current_total = actual_dl_size;
    static time_t start_time = time(NULL);
    // Watch actual_dl_size to see if the file changes
    if (current_total != actual_dl_size) {
	current_total = actual_dl_size;
	start_time = time(NULL);
    }
    time_t current_time = time(NULL);
    time_t transfer_time = start_time - current_time;
    double fraction_downloaded = actual_now_dl / actual_dl_size;
    /*
     * Calculate time remaining & download speed, in seconds & bytes
     * respectively. Here we must use the numbers without resume_point,
     * otherwise we get wrong results when resuming a download.
     */
    double download_eta = transfer_time - (transfer_time / (now_downloaded / total_to_download) );
    double current_dl_speed = (now_downloaded / transfer_time) * -1; // We get negative values back

    /* Determine what units we should use */
    const char *dlunit;
    const char *ndunit;
    const char *spunit;
    double total_size;
    double downloaded;
    double dlspeed;

    // Download size
    if (actual_dl_size >= TBYTE) {
	dlunit = "TiB";
	total_size = actual_dl_size / TBYTE;
    } else if (actual_dl_size >= GBYTE) {
	dlunit = "GiB";
	total_size = actual_dl_size / GBYTE;
    } else if (actual_dl_size >= MBYTE) {
	dlunit = "MiB";
	total_size = actual_dl_size / MBYTE;
    } else if (actual_dl_size >= KBYTE) {
	dlunit = "KiB";
	total_size = actual_dl_size / KBYTE;
    } else {
	dlunit = "B";
	total_size = actual_dl_size;
    }

    // Downloaded size
    if (actual_now_dl >= TBYTE) {
	ndunit = "TiB";
	downloaded = actual_now_dl / TBYTE;
    } else if (actual_now_dl >= GBYTE) {
	ndunit = "GiB";
	downloaded = actual_now_dl / GBYTE;
    } else if (actual_now_dl >= MBYTE) {
	ndunit = "MiB";
	downloaded = actual_now_dl / MBYTE;
    } else if (actual_now_dl >= KBYTE) {
	ndunit = "KiB";
	downloaded = actual_now_dl / KBYTE;
    } else {
	ndunit = "B";
	downloaded = actual_now_dl;
    }

    // Speed
    if (current_dl_speed >= TBYTE) {
	spunit = "TiB/s";
	dlspeed = current_dl_speed / TBYTE;
    } else if (current_dl_speed >= GBYTE) {
	spunit = "GiB/s";
	dlspeed = current_dl_speed / GBYTE;
    } else if (current_dl_speed >= MBYTE) {
	spunit = "MiB/s";
	dlspeed = current_dl_speed / MBYTE;
    } else if (current_dl_speed >= KBYTE) {
	spunit = "KiB/s";
	dlspeed = current_dl_speed / KBYTE;
    } else {
	spunit = "B/s";
	dlspeed = current_dl_speed;
    }

    // Time left
    int secs=0, mins=0, hours=0, days=0;
    int n = (int)download_eta;
    if (n >= DAY) {
	days = n / DAY;
	n = n % DAY;
    }
    if (n >= HOUR) {
	hours = n / HOUR;
	n = n % HOUR;
    }
    if (n >= MINUTE) {
	mins = n / MINUTE;
	n = n % MINUTE;
    }
    secs = n;
    char timeleft[16];
    int cx = 0;
    if (days && cx >=0 && cx < 16)
	cx = snprintf(timeleft+cx, sizeof(timeleft), "%dD:", days);
    if (hours && cx >=0 && cx < 16)
	cx = snprintf(timeleft+cx, sizeof(timeleft), (hours < 10) ? "0%d:" : "%d:", hours);
    if (mins && cx >=0 && cx < 16)
	cx = snprintf(timeleft+cx, sizeof(timeleft), (mins < 10) ? "0%d:" : "%d:", mins);
    if (!days && !hours && !mins && cx >=0 && cx < 16)  // Keep showing the minutes part
	snprintf(timeleft+cx, sizeof(timeleft), (secs < 10) ? "00:0%d" : "00:%d", secs);
    else if (cx >= 0 && cx < 16)
	snprintf(timeleft+cx, sizeof(timeleft), (secs < 10) ? "0%d" : "%d", secs);

    /* Progress bar */
    int totaldots = 30;
    // part of the progress bar that's already full
    int dots = (int)(round(fraction_downloaded * totaldots));

    // create the meter
    printf("%c[2K", 27);  // Clears the previously written line
    int i;
    printf("%3.0f%% [", fraction_downloaded*100);
    for (i=0; i < dots; i++) {
	printf("=");
    }
    for (; i < totaldots; i++) {
	printf(" ");
    }
    if (isinf(dlspeed) || isinf(download_eta) || isnan(secs) || secs < 0)
	printf("] %.2f %s / %.2f %s\r", downloaded, ndunit, total_size, dlunit);
    else
	printf("] %.2f %s / %.2f %s (%.2f %s) [%s left]\r",
	       downloaded, ndunit, total_size, dlunit, dlspeed, spunit, timeleft);
    fflush(stdout);

    // Must return 0 otherwise the transfer is aborted
    return 0;
}
