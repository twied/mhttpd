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

#include <cstdio>       /* std::perror() */
#include <cstdlib>      /* std::exit() */
#include <ctime>        /* std::time() */
#include <iostream>     /* std::cout */
#include <sstream>      /* std::stringstream */
#include <vector>       /* std::vector */

#include <netdb.h>      /* accept(), send(), shutdown() recv() */
#include <unistd.h>     /* fork(), close() */
#include <wait.h>       /* sig_atomic_t, signal(), waitpid() */

#include "mhttpd.h"

namespace mhttpd {

class Request::Implementation {
public:
    Implementation(const int sock) :
            sock(sock) {
    }

    ~Implementation() {
    }

    /** Unix socket to read from. */
    const int sock;
};

Request::Request(const int sock) :
        implementation(new Implementation(sock)) {
    port = ip[0] = ip[1] = ip[2] = ip[3] = 0;
}

Request::~Request() {
    delete implementation;
}

size_t Request::get(char& c) {
    return read(&c, sizeof c);
}

size_t Request::read(char* buffer, size_t length) {
    return recv(implementation->sock, buffer, length, 0);
}

class Response::Implementation {
public:
    Implementation(const int sock) :
            sock(sock), headerSent(false) {
    }

    ~Implementation() {
        if (!sendBuffer.empty()) {
            writeBuffer(sendBuffer.data(), sendBuffer.size());
        }
    }

    void sendHeader(Response& response) {
        headerSent = true;
        response << response.version << " " << response.statusCode << " " << response.statusMessage << "\r\n";
        response << "Content-Type: " << response.contentType << "\r\n";
        response << "Connection: Close\r\n";

        for (std::map<std::string, std::string>::const_iterator it = response.fields.begin(); it != response.fields.end(); ++it) {
            response << it->first << ": " << it->second << "\r\n";
        }

        response << "\r\n";
    }

    void write(const char* buffer, size_t length) {
        if (sendBuffer.empty()) {
            if (length >= BUFSIZ) {
                /* empty sendBuffer && big buffer => send directly */
                writeBuffer(buffer, length);
            } else {
                /* empty sendBuffer && small buffer => put in sendBuffer */
                for (size_t i = 0; i < length; ++i) {
                    sendBuffer.push_back(buffer[i]);
                }
            }

            return;
        }

        if (sendBuffer.size() + length < BUFSIZ) {
            /* sum(data) is small => put in sendBuffer */
            for (size_t i = 0; i < length; ++i) {
                sendBuffer.push_back(buffer[i]);
            }
        } else {
            /* sum(data) is big => empty sendBuffer and try again */
            writeBuffer(sendBuffer.data(), sendBuffer.size());
            sendBuffer.clear();
            write(buffer, length);
        }
    }

    /** Unix socket to write to. */
    const int sock;

    /** Set if HTTP header was sent. */
    bool headerSent;

private:
    std::vector<char> sendBuffer;

    void writeBuffer(const char* buffer, size_t length) {
        size_t offset = 0;
        while (offset < length) {
            ssize_t bytes = send(sock, buffer + offset, length - offset, 0);
            if (bytes < 0) {
                break;
            }
            offset += bytes;
        }
    }
};

Response::Response(const int sock) :
        version("HTTP/1.1"), statusCode(501), statusMessage("Not Implemented"), contentType("application/octet-stream"), implementation(new Implementation(sock)) {
}

Response::~Response() {
    if (!implementation->headerSent) {
        implementation->sendHeader(*this);
    }
    delete implementation;
}

Response& Response::put(const char c) {
    return write(&c, sizeof c);
}

Response& Response::write(const char* buffer, size_t length) {
    if (!implementation->headerSent) {
        implementation->sendHeader(*this);
    }

    implementation->write(buffer, length);
    return *this;
}

Response& operator<<(Response& response, const std::string& string) {
    return response.write(string.c_str(), string.length());
}

Response& operator<<(Response& response, const int& i) {
    std::stringstream stream;
    stream << i;
    return response << stream.str();
}

class Log::Implementation {
public:
    Implementation() :
            stream() {
    }

    ~Implementation() {
        std::cout << stream.str() << std::endl;
    }

    void writeTime() {
        char string[100];
        std::time_t now = std::time(NULL);
        std::strftime(string, sizeof(string), "%c: ", std::localtime(&now));
        stream << string;
    }

    void writeAddress(const unsigned char ip[4], const unsigned short port) {
        stream << "(" << (int) ip[0] << "." << (int) ip[1] << "." << (int) ip[2] << "." << (int) ip[3] << ":" << (int) port << ") ";
    }

    std::stringstream stream;
};

Log::Log() :
        implementation(new Implementation) {
    implementation->writeTime();
}

Log::Log(const Request& request) :
        implementation(new Implementation) {
    implementation->writeTime();
    implementation->writeAddress(request.ip, request.port);
}

Log::~Log() {
    delete implementation;
}

Log& Log::put(const char c) {
    implementation->stream.put(c);
    return *this;
}

Log& Log::write(const char* s, size_t n) {
    implementation->stream.write(s, n);
    return *this;
}

Log& Log::operator<<(const int& value) {
    implementation->stream << value;
    return *this;
}

Log& Log::operator<<(const float& value) {
    implementation->stream << value;
    return *this;
}

Log& Log::operator<<(const double& value) {
    implementation->stream << value;
    return *this;
}

Log& Log::operator<<(const std::string& value) {
    implementation->stream.write(value.c_str(), value.length());
    return *this;
}

static int server_socket_fd;

static volatile sig_atomic_t server_running = 1;

static void server_signal(int) {
    close(server_socket_fd);
    server_running = 0;
}

static void server_worker(int sock, struct sockaddr_in* sockaddr, handler_t handler) {
    /* double fork to avoid zombie processes */
    pid_t pid = fork();
    switch (pid) {
    case -1:
        /* error */
        std::perror("double fork() failed");
        return;

    case 0:
        /* child */
        break;

    default:
        /* parent */
        std::exit(0);
        break;
    }

    /* set timeout to 5 seconds */
    struct timeval timeval = {5, 0};
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeval, sizeof(timeval)) < 0) {
        std::perror("setsockopt(rcvtimeo) failed");
        return;
    }

    char buffer[BUFSIZ] = {0};
    size_t length = 0;

    do {
        ssize_t read = recv(sock, buffer + length, (BUFSIZ - length) > 0 ? 1 : 0, 0);

        if (read == 0) {
            /* connection closed => close connection */
            return;
        }

        if (read < 0) {
            /* timeout / another error => close connection */
            return;
        }

        length += read;

        if (length >= BUFSIZ) {
            /* maximum request size reached => close connection */
            return;
        }
    } while (length < 4 || buffer[length - 4] != '\r' || buffer[length - 3] != '\n' || buffer[length - 2] != '\r' || buffer[length - 1] != '\n');

    size_t pos = 0;
    Request request(sock);
    Response response(sock);

    /* parse request type */
    while (pos < length && buffer[pos] != ' ') {
        request.type += buffer[pos];
        pos += 1;
    }

    /* should be a space here. illegal request? => close connection */
    if (pos < length && buffer[pos] == ' ') {
        pos += 1;
    } else {
        return;
    }

    /* parse request path */
    while (pos < length && buffer[pos] != ' ') {
        request.path += buffer[pos];
        pos += 1;
    }

    /* should be a space here. illegal request? => close connection */
    if (pos < length && buffer[pos] == ' ') {
        pos += 1;
    } else {
        return;
    }

    /* parse request http version */
    while (pos < length && buffer[pos] != '\r') {
        request.version += buffer[pos];
        pos += 1;
    }

    /* should be "\r\n" here. illegal request? => close connection */
    if (pos < length - 1 && buffer[pos] == '\r' && buffer[pos + 1] == '\n') {
        pos += 2;
    } else {
        return;
    }

    /* parse header fields */
    while (pos < length - 2) {
        std::string key;
        std::string value;

        while (pos < length && buffer[pos] != ':') {
            key += buffer[pos];
            pos += 1;
        }

        /* should be ':' here. illegal request? => close connection */
        if (pos < length && buffer[pos] == ':') {
            pos += 1;
        } else {
            return;
        }

        /* remove whitespace before content */
        while (pos < length && (buffer[pos] == ' ' || buffer[pos] == '\t')) {
            pos += 1;
        }

        while (pos < length && buffer[pos] != '\r') {
            value += buffer[pos];
            pos += 1;
        }

        /* should be "\r\n" here. illegal request? => close connection */
        if (pos < length - 1 && buffer[pos] == '\r' && buffer[pos + 1] == '\n') {
            pos += 2;
        } else {
            return;
        }

        request.fields[key] = value;
    }

    /* parse path parameters */
    if ((pos = request.path.find('?')) != std::string::npos) {
        std::string query = request.path.substr(pos + 1, std::string::npos);
        request.path.erase(pos);

        while (!query.empty()) {
            std::string key;
            std::string val;

            pos = query.find('&');
            key = query.substr(0, pos);
            query.erase(0, pos);
            query.erase(0, 1);

            if ((pos = key.find('=')) != std::string::npos) {
                val = key.substr(pos + 1, std::string::npos);
                key.erase(pos);
            }

            request.parameters.insert(std::pair<std::string, std::string>(urldecode(key), urldecode(val)));
        }
    }

    request.path = urldecode(request.path);
    request.port = ntohs(sockaddr->sin_port);
    request.ip[0] = 0xff & (sockaddr->sin_addr.s_addr >> 0);
    request.ip[1] = 0xff & (sockaddr->sin_addr.s_addr >> 8);
    request.ip[2] = 0xff & (sockaddr->sin_addr.s_addr >> 16);
    request.ip[3] = 0xff & (sockaddr->sin_addr.s_addr >> 24);

    handler(request, response);
}

int start(unsigned port, handler_t handler) {
    /* set up signal handler */
    if (SIG_ERR == signal(SIGINT, server_signal)) {
        std::perror("signal() failed");
        return 1;
    }

    /* create socket */
    if (-1 == (server_socket_fd = socket(AF_INET, SOCK_STREAM, 0))) {
        std::perror("socket() failed");
        return 1;
    }

    /* bind socket to local address */
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (-1 == bind(server_socket_fd, (struct sockaddr *) &addr, sizeof(struct sockaddr))) {
        std::perror("bind() failed");
        return 1;
    }

    /* mark socket as listening socket */
    if (-1 == listen(server_socket_fd, SOMAXCONN)) {
        std::perror("listen() failed");
        return 1;
    }

    /* accept connection and process */
    while (server_running) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_length = sizeof(client_addr);
        int socket_fd = accept(server_socket_fd, (struct sockaddr*) &client_addr, &client_addr_length);
        if (socket_fd < 0) {
            if (!server_running) {
                break;
            }

            std::perror("accept() failed");
            continue;
        }

        pid_t pid = fork();
        switch (pid) {
        case -1:
            /* error */
            std::perror("fork() failed");
            return 1;

        case 0:
            /* child */
            close(server_socket_fd);
            server_worker(socket_fd, &client_addr, handler);
            shutdown(socket_fd, SHUT_RDWR);
            close(socket_fd);
            std::exit(0);

        default:
            /* parent */
            close(socket_fd);
            waitpid(pid, &socket_fd, 0);
        }
    }

    /* shutdown */
    shutdown(server_socket_fd, SHUT_RDWR);
    close(server_socket_fd);
    return 0;
}

std::string urlencode(const std::string& s) {
    std::stringstream stream;
    stream.fill('0');
    stream << std::hex;

    for (size_t i = 0; i < s.length(); ++i) {
        const char c = s[i];

        if (c >= 'a' && c <= 'z') {
            stream << c;
        } else if (c >= 'A' && c <= 'Z') {
            stream << c;
        } else if (c >= '0' && c <= '9') {
            stream << c;
        } else if (c == '-' || c == '_' || c == '.') {
            stream << c;
        } else if (c == ' ') {
            stream << '+';
        } else {
            stream.width(2);
            stream << static_cast<unsigned>(c);
        }
    }

    return stream.str();
}

static int hexvalue(const char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 0x0a;
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 0x0a;
    } else {
        return -1;
    }
}

std::string urldecode(const std::string& s) {
    std::stringstream stream;

    for (size_t i = 0; i < s.length(); ++i) {
        const char c = s[i];

        if (c == '+') {
            stream << ' ';
        } else if (c == '%' && i + 2 < s.length()) {
            const int hi = hexvalue(s[i + 1]);
            const int lo = hexvalue(s[i + 2]);
            if (hi < 0 || lo < 0) {
                stream << '%';
            } else {
                stream << static_cast<char>(hi << 4 | lo);
                i += 2;
            }
        } else {
            stream << c;
        }
    }

    return stream.str();
}

std::string htmlspecialchars(const std::string& s) {
    std::stringstream stream;

    for (size_t i = 0; i < s.length(); ++i) {
        const char c = s[i];

        switch (c) {
        case '&':
            stream << "&amp;";
            break;
        case '<':
            stream << "&lt;";
            break;
        case '>':
            stream << "&gt;";
            break;
        default:
            stream << c;
            break;
        }
    }

    return stream.str();
}

std::string sanitizepath(const std::string& path) {
    std::vector<std::string> vector;
    std::stringstream ss(path);
    std::string string;

    /* tokenize path */
    while (std::getline(ss, string, '/')) {
        vector.push_back(string);
    }

    /* reassemble path */
    string = std::string();
    bool ignore = false;
    for (std::vector<std::string>::reverse_iterator it = vector.rbegin(); it != vector.rend(); ++it) {
        if (ignore) {
            ignore = false;
        } else if (it->empty()) {
            continue;
        } else if (*it == ".") {
            continue;
        } else if (*it == "..") {
            ignore = true;
        } else if (string.empty()) {
            string = *it;
        } else {
            string = *it + "/" + string;
        }
    }

    /* preserve trailing '/' */
    if (path[path.size() - 1] == '/') {
        string += '/';
    }

    return "/" + string;
}

} /* namespace mhttpd */
