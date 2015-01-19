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

#ifndef MHTTPD_H_
#define MHTTPD_H_

#include <map>      /* std::map */
#include <string>   /* std::string */

namespace mhttpd {

/** HTTP request class. */
class Request {
public:
    /**
     * Create a new request.
     * @param sock Unix socket to read from
     */
    Request(const int sock);

    /** Destroy this request. */
    ~Request();

    /** Client IP address, 0 if unknown. */
    unsigned char ip[4];

    /** Client port, 0 if unknown. */
    unsigned short port;

    /* HTTP request type, see RFC 7231 section 4.3. */
    std::string type;

    /** HTTP version, see RFC 2145. */
    std::string version;

    /** Requested path. */
    std::string path;

    /** Header fields. */
    std::map<std::string, std::string> fields;

    /** Request parameters. */
    std::multimap<std::string, std::string> parameters;

    /** Get a character from the input. */
    size_t get(char&);

    /** Read a block of data from the input. */
    size_t read(char*, size_t);

private:
    /** No copy constructor. */
    Request(const Request&);

    /** No copy assignment. */
    Request operator=(const Request&);

    class Implementation;
    Implementation* const implementation;
};

/** HTTP response class. */
class Response {
public:
    /**
     * Create a new response.
     * @param sock Unix socket to write to
     */
    Response(const int sock);

    /** Destroy this response. */
    ~Response();

    /** HTTP version, see RFC 2145. */
    std::string version;

    /** HTTP status code, see RFC 7231 section 6. */
    unsigned statusCode;

    /** HTTP status message, see RFC 7231 section 6. */
    std::string statusMessage;

    /** Content type, see RFC 7231 section 7. */
    std::string contentType;

    /** Header fields, see RFC 7231 section 7. */
    std::map<std::string, std::string> fields;

    /** Add a character to the output. */
    Response& put(const char);

    /** Add a block of data to the output. */
    Response& write(const char*, size_t);

    /** Add a string to the output. */
    friend Response& operator<<(Response&, const std::string&);

    /** Add an int to the output. */
    friend Response& operator<<(Response&, const int&);

private:
    /** No copy constructor. */
    Response(const Response&);

    /** No copy assignment. */
    Response operator=(const Response&);

    class Implementation;
    Implementation* const implementation;
};

/**
 * Logging facility.
 * Usage: {@code Log() << "message";}
 */
class Log {
public:
    Log();

    Log(const Request& request);

    ~Log();

    /* Insert a character. */
    Log& put(const char c);

    /* Insert the first n characters of the array pointed by s. */
    Log& write(const char* s, size_t n);

    Log& operator<<(const int& value);

    Log& operator<<(const float& value);

    Log& operator<<(const double& value);

    Log& operator<<(const std::string& value);

private:
    /** No copy constructor. */
    Log(const Log&);

    /** No copy assignment. */
    Log operator=(const Log&);

    class Implementation;
    Implementation* const implementation;
};

/** Type definition of a mhttp handler. */
typedef void (*handler_t)(const Request&, Response&);

/**
 * Start mhttpd server.
 * @param port local port to listen on
 * @param handler call back function for incoming request
 * @return non-zero value on failure, 0 on termination by SIGINT.
 */
int start(unsigned port, handler_t handler);

/**
 * Utility function to convert all non-alphanumerical characters to %## and
 * " " to "+", similar to php's urlencode function.
 */
std::string urlencode(const std::string& s);

/**
 * Utility function to decode all %## encodings and "+" from urlencode(),
 * similar to php's urldecode function.
 */
std::string urldecode(const std::string& s);

/**
 * Utility function to convert special characters to html entities, similar to
 * php's htmlspecialchars function.
 */
std::string htmlspecialchars(const std::string& s);

/**
 * Utility function to sanitize paths.
 * Removes "anything/../" and "/./" parts of a path. Any path going above "/"
 * will be reduced to "/".
 */
std::string sanitizepath(const std::string& path);

} /* namespace mhttpd */

#endif /* MHTTPD_H_ */
