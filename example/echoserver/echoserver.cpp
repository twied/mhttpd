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

#define PORT 8080

static void handle(const mhttpd::Request& q, mhttpd::Response& r) {
    mhttpd::Log() << q.type << " " << q.path;

    r.statusCode = 200;
    r.statusMessage = "OK";
    r.contentType = "text/html; charset=ISO-8859-1";
    r << "<!doctype html>\n";
    r << "<html>\n";
    r << " <head>\n";
    r << "  <title>mhttpd demo</title>\n";
    r << " </head>\n";
    r << " <body>\n";
    r << "  <h1>mhttpd demo</h1>\n";
    r << "  <p>Your IP and port: " << q.ip[0] << "." << q.ip[1] << "." << q.ip[2] << "." << q.ip[3] << ":" << q.port << "</p>\n";
    r << "  <p>Your request was of type " << q.type << "</p>\n";
    r << "  <p>Your request version was " << q.version << "</p>\n";
    r << "  <p>Your request path was " << mhttpd::htmlspecialchars(q.path) << "</p>\n";
    r << "  <p>Parameter:\n";
    r << "   <ul>\n";
    for (std::multimap<std::string, std::string>::const_iterator it = q.parameters.begin(); it != q.parameters.end(); ++it) {
        r << "    <li>" << mhttpd::htmlspecialchars(it->first) << " = " << mhttpd::htmlspecialchars(it->second) << "</li>\n";
    }
    r << "   </ul>\n";
    r << "  <p>Header fields:\n";
    r << "   <ul>\n";
    for (std::map<std::string, std::string>::const_iterator it = q.fields.begin(); it != q.fields.end(); ++it) {
        r << "    <li>" << mhttpd::htmlspecialchars(it->first) << " = " << mhttpd::htmlspecialchars(it->second) << "</li>\n";
    }
    r << "   </ul>\n";
    r << "  </p>\n";
    r << " </body>\n";
    r << "</html>\n";
}

int main() {
    mhttpd::Log() << "Server started on Port " << PORT;
    return mhttpd::start(PORT, handle);
}
