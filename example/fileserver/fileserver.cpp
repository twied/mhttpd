/*
 * Copyright (c) 2015, Tim Wiederhake
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <mhttpd.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define PORT 8080

const char* basepath;

static bool endsWith(const std::string& path, const std::string& suffix) {
    if (suffix.length() > path.length()) {
        return false;
    }

    std::string::const_reverse_iterator it_suffix = suffix.rbegin();
    std::string::const_reverse_iterator it_path = path.rbegin();
    for (; it_suffix != suffix.rend(); ++it_suffix, ++it_path) {
        if (*it_suffix != *it_path) {
            return false;
        }
    }

    return true;
}

static void setContentType(const std::string& path, mhttpd::Response& r) {
    if (endsWith(path, ".htm") || endsWith(path, ".html") || endsWith(path, ".shtml") || endsWith(path, ".xhtml")) {
        r.contentType = "text/html";
    } else if (endsWith(path, ".xml")) {
        r.contentType = "text/xml";
    } else if (endsWith(path, ".css")) {
        r.contentType = "text/css";
    } else if (endsWith(path, ".js")) {
        r.contentType = "text/javascript";
    } else if (endsWith(path, ".txt")) {
        r.contentType = "text/plain";
    } else if (endsWith(path, ".pdf")) {
        r.contentType = "application/pdf";
    } else if (endsWith(path, ".jpg") || endsWith(path, ".jpeg") || endsWith(path, ".jpe")) {
        r.contentType = "image/jpeg";
    } else if (endsWith(path, ".gif")) {
        r.contentType = "image/gif";
    } else if (endsWith(path, ".png")) {
        r.contentType = "image/png";
    } else if (endsWith(path, ".ico")) {
        r.contentType = "image/x-icon";
    }
}

static bool handle(const std::string& path, mhttpd::Response& r) {
    struct stat buf;

    if (stat(path.c_str(), &buf) != 0) {
        /* path does not exist */
        return false;
    }

    if ((buf.st_mode & S_IFMT) == S_IFREG) {
        /* is a file */
        setContentType(path, r);
        std::stringstream contentLength;
        contentLength << buf.st_size;
        r.fields["Content-Length"] = contentLength.str();
        r.statusCode = 200;
        r.statusMessage = "OK";

        std::ifstream file(path.c_str());
        int i;
        while ((i = file.get()) != std::char_traits<char>::eof()) {
            r.put(i);
        }

        return true;
    }

    if ((buf.st_mode & S_IFMT) == S_IFDIR) {
        /* is a directory */
        if (handle(path + "/index.htm", r)) {
            return true;
        }

        if (handle(path + "/index.html", r)) {
            return true;
        }

        if (handle(path + "/index.shtml", r)) {
            return true;
        }

        return false;
    }

    /* is something else */
    return false;
}

static void handle(const mhttpd::Request& q, mhttpd::Response& r) {
    mhttpd::Log(q) << q.type << " " << q.path;
    if (handle(basepath + mhttpd::sanitizepath(q.path), r)) {
        return;
    }

    r.statusCode = 404;
    r.statusMessage = "Not Found";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        mhttpd::Log() << "Usage: " << argv[0] << " BASEDIRECTORY";
        return 1;
    }

    basepath = argv[1];

    mhttpd::Log() << "Server started on Port " << PORT;
    return mhttpd::start(PORT, handle);
}
