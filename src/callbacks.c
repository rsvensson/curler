#include "callbacks.h"

#include <math.h>
#include <stdio.h>
#include <time.h>

#define KBYTE  double(1024)
#define MBYTE (double(1024) * KBYTE)
#define GBYTE (double(1024) * MBYTE)
#define TBYTE (double(1024) * GBYTE)

#define MINUTE double(60)
#define HOUR  (double(60) * MINUTE)
#define DAY   (double(24) * HOUR)

/* custom callback function for CURLOPT_WRITEFUNCTION. */
size_t write_callback(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}


/*
 * Custom callback function for CURLOPT_HEADERFUNCTION to extract header data.
 * Currently only used to get the filename if available.
 */
size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
    char *cd = (char *)userdata;

    // TODO: this leaves a trailing " character for some reason even though it's escaped
    sscanf(buffer, "content-disposition: %*s %*s filename=\"%s\"", cd);

    return nitems * size;
}


/* custom callback function for CURLOPT_PROGRESSFUNCTION to create a progress bar. */
int progress_callback(void *ptr, double total_to_download, double now_downloaded,
			 double total_to_upload, double now_uploaded)
{
    /* Make sure the file is not empty */
    if (total_to_download <= 0.0)
	return 0;

    /* For measuring download speed & time remaining */
    static double current_total = total_to_download;
    static time_t start_time = time(NULL);
    // Watch total_to_download to see if the file changes
    if (current_total != total_to_download) {
	current_total = total_to_download;
	start_time = time(NULL);
    }
    time_t current_time = time(NULL);
    time_t transfer_time = start_time - current_time;
    double fraction_downloaded = now_downloaded / total_to_download;
    // Calculate time remaining
    double download_eta = transfer_time - (transfer_time / fraction_downloaded);
    // Calculate download speed
    double current_dl_speed = (now_downloaded / transfer_time) * -1; // We get negative values back

    /* Determine what units we should use */
    const char *dlunit;
    const char *ndunit;
    const char *spunit;
    double total_size;
    double downloaded;
    double dlspeed;

    // Download size
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

    // Downloaded size
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
    double days=0, hours=0, mins=0, secs=0;
    if (download_eta >= DAY) {
	days  = download_eta / DAY;
	hours = days / HOUR;
	mins  = hours / MINUTE;
	secs  = (int)mins % (int) MINUTE;
    } else if (download_eta >= HOUR) {
	hours = download_eta / HOUR;
	mins  = hours / MINUTE;
	secs  = (int)mins % (int) MINUTE;
    } else if (download_eta >= MINUTE) {
	mins = download_eta / MINUTE;
	secs = (int)download_eta % (int)MINUTE;
    } else
	secs = download_eta;
    // Format output of time
    char timeleft[32];
    int cx = 0;
    if (days && cx >=0 && cx < 32)
	cx = snprintf(timeleft+cx, sizeof(timeleft), "%.0fD:", days);
    if (hours && cx >=0 && cx < 32)
	cx = snprintf(timeleft+cx, sizeof(timeleft), (hours < 10) ? "0%1.0f:" : "%2.0f:", hours);
    if (mins && cx >=0 && cx < 32)
	cx = snprintf(timeleft+cx, sizeof(timeleft), (mins < 10) ? "0%1.0f:" : "%2.0f:", mins);
    if (!days && !hours && !mins && cx >=0 && cx < 32)  // Keep showing the minutes part
	snprintf(timeleft+cx, sizeof(timeleft), (secs < 10) ? "00:0%1.0f" : "00:%2.0f", secs);
    else if (cx >= 0 && cx < 32)
	snprintf(timeleft+cx, sizeof(timeleft), (secs < 10) ? "0%1.0f" : "%2.0f", secs);

    /* Progress bar */
    int totaldots = 40;
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
