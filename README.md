# curler
A simple downloader using libcurl

## Features
* Add downloads to a queue which is processed sequentially (multi-download may come in the future)
* Specify which downloads are downloaded to which path
* Tries to determine filename automatically if not provided
* Simple progress bar output

## Install

    git clone https://github.com/rsvensson/curler.git
    cd curler
    make

## Usage
The basic usage is:

    curler -p <path> -u <url> <name>

You can specify multiple paths and urls. Every url after a path flag is downloaded into that path. Specifying the filename is optional.

    curler -p <path1> -u <url1> <name1> -u <url2> -p <path2> -u <url3> <name3>

You can also specify a text file to pull the urls and filenames from.

    curler -p <path> -f <file>
The text file should have the following format:

    <url1> <name1>
    <url2> <name2>
    <url3>
    ...

Of course you can combine files and urls as well

    curler -p <path1> -f <file1> -u <url1> <name> -p <path2> -f <file2> -f <file3> -u <url2>

## Options

    -f <file.txt>     Read
    -p <path>         Path that you want to download the urls following this flag to
    -u <url> [<name>] URL to download, with optional filename.
