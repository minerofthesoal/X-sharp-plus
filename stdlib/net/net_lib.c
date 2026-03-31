/*
 * X# Standard Library - Net Module Implementation
 * ==================================================
 * POSIX socket-based networking.
 */

#include "net_lib.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

/* ===== Helpers ===== */

static int parse_url(const char* url, char* host, int host_sz,
                     char* path, int path_sz, int* port, int* use_https) {
    *port = 80;
    *use_https = 0;
    const char* p = url;
    if (strncmp(p, "https://", 8) == 0) { p += 8; *port = 443; *use_https = 1; }
    else if (strncmp(p, "http://", 7) == 0) { p += 7; }

    const char* slash = strchr(p, '/');
    const char* colon = strchr(p, ':');

    int hlen;
    if (colon && (!slash || colon < slash)) {
        hlen = (int)(colon - p);
        *port = atoi(colon + 1);
    } else if (slash) {
        hlen = (int)(slash - p);
    } else {
        hlen = (int)strlen(p);
    }
    if (hlen >= host_sz) hlen = host_sz - 1;
    memcpy(host, p, hlen);
    host[hlen] = '\0';

    if (slash) {
        strncpy(path, slash, path_sz - 1);
        path[path_sz - 1] = '\0';
    } else {
        strcpy(path, "/");
    }
    return 0;
}

static int tcp_connect(const char* host, int port) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);

    if (getaddrinfo(host, port_str, &hints, &res) != 0) return -1;

    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) { freeaddrinfo(res); return -1; }

    if (connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
        close(fd); freeaddrinfo(res); return -1;
    }
    freeaddrinfo(res);
    return fd;
}

/* ===== httpGet(url) -> scroll ===== */

XsValue xs_net_httpGet(int argc, XsValue* args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_abyss();

    char host[256], path[2048];
    int port, use_https;
    parse_url(args[0].scroll, host, sizeof(host), path, sizeof(path), &port, &use_https);

    /* Note: HTTPS requires TLS which is not handled here; HTTP only */
    int fd = tcp_connect(host, port);
    if (fd < 0) return xs_abyss();

    /* Send HTTP request */
    char req[4096];
    int req_len = snprintf(req, sizeof(req),
        "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host);
    send(fd, req, req_len, 0);

    /* Read response */
    size_t cap = 16384, len = 0;
    char* buf = (char*)malloc(cap);
    ssize_t n;
    while ((n = recv(fd, buf + len, cap - len - 1, 0)) > 0) {
        len += n;
        if (len + 1 >= cap) { cap *= 2; buf = (char*)realloc(buf, cap); }
    }
    close(fd);
    buf[len] = '\0';

    /* Skip HTTP headers */
    char* body = strstr(buf, "\r\n\r\n");
    if (body) {
        body += 4;
        char* result = xs_strdup(body);
        free(buf);
        return xs_scroll(result);
    }
    return xs_scroll(buf);
}

/* ===== httpPost(url, body, contentType?) -> scroll ===== */

XsValue xs_net_httpPost(int argc, XsValue* args) {
    if (argc < 2 || args[0].type != VAL_SCROLL || args[1].type != VAL_SCROLL) return xs_abyss();

    char host[256], path[2048];
    int port, use_https;
    parse_url(args[0].scroll, host, sizeof(host), path, sizeof(path), &port, &use_https);

    const char* content_type = "application/x-www-form-urlencoded";
    if (argc >= 3 && args[2].type == VAL_SCROLL) content_type = args[2].scroll;

    int fd = tcp_connect(host, port);
    if (fd < 0) return xs_abyss();

    size_t body_len = strlen(args[1].scroll);
    char header[4096];
    int hlen = snprintf(header, sizeof(header),
        "POST %s HTTP/1.1\r\nHost: %s\r\nContent-Type: %s\r\n"
        "Content-Length: %zu\r\nConnection: close\r\n\r\n",
        path, host, content_type, body_len);
    send(fd, header, hlen, 0);
    send(fd, args[1].scroll, body_len, 0);

    size_t cap = 16384, len = 0;
    char* buf = (char*)malloc(cap);
    ssize_t n;
    while ((n = recv(fd, buf + len, cap - len - 1, 0)) > 0) {
        len += n;
        if (len + 1 >= cap) { cap *= 2; buf = (char*)realloc(buf, cap); }
    }
    close(fd);
    buf[len] = '\0';

    char* body = strstr(buf, "\r\n\r\n");
    if (body) {
        body += 4;
        char* result = xs_strdup(body);
        free(buf);
        return xs_scroll(result);
    }
    return xs_scroll(buf);
}

/* ===== tcpConnect(host, port) -> blade (fd) ===== */

XsValue xs_net_tcpConnect(int argc, XsValue* args) {
    if (argc < 2 || args[0].type != VAL_SCROLL) return xs_blade(-1);
    int port = (int)xs_as_spark(args[1]);
    int fd = tcp_connect(args[0].scroll, port);
    return xs_blade((int64_t)fd);
}

/* ===== tcpListen(port) -> blade (fd) ===== */

XsValue xs_net_tcpListen(int argc, XsValue* args) {
    if (argc < 1) return xs_blade(-1);
    int port = (int)xs_as_spark(args[0]);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return xs_blade(-1);

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd); return xs_blade(-1);
    }
    if (listen(fd, 128) < 0) {
        close(fd); return xs_blade(-1);
    }
    return xs_blade((int64_t)fd);
}

/* ===== tcpAccept(fd) -> blade (client fd) ===== */

XsValue xs_net_tcpAccept(int argc, XsValue* args) {
    if (argc < 1) return xs_blade(-1);
    int fd = (int)xs_as_spark(args[0]);
    struct sockaddr_in client;
    socklen_t clen = sizeof(client);
    int cfd = accept(fd, (struct sockaddr*)&client, &clen);
    return xs_blade((int64_t)cfd);
}

/* ===== tcpSend(fd, data) -> blade (bytes sent) ===== */

XsValue xs_net_tcpSend(int argc, XsValue* args) {
    if (argc < 2 || args[1].type != VAL_SCROLL) return xs_blade(-1);
    int fd = (int)xs_as_spark(args[0]);
    size_t len = strlen(args[1].scroll);
    ssize_t sent = send(fd, args[1].scroll, len, 0);
    return xs_blade((int64_t)sent);
}

/* ===== tcpRecv(fd, maxBytes?) -> scroll ===== */

XsValue xs_net_tcpRecv(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    int fd = (int)xs_as_spark(args[0]);
    int max_bytes = (argc >= 2) ? (int)xs_as_spark(args[1]) : 4096;
    char* buf = (char*)malloc(max_bytes + 1);
    ssize_t n = recv(fd, buf, max_bytes, 0);
    if (n <= 0) { free(buf); return xs_abyss(); }
    buf[n] = '\0';
    return xs_scroll(buf);
}

/* ===== tcpClose(fd) ===== */

XsValue xs_net_tcpClose(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    int fd = (int)xs_as_spark(args[0]);
    close(fd);
    return xs_abyss();
}

/* ===== udpSend(host, port, data) -> blade (bytes sent) ===== */

XsValue xs_net_udpSend(int argc, XsValue* args) {
    if (argc < 3 || args[0].type != VAL_SCROLL || args[2].type != VAL_SCROLL) return xs_blade(-1);
    int port = (int)xs_as_spark(args[1]);

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return xs_blade(-1);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    struct hostent* he = gethostbyname(args[0].scroll);
    if (!he) { close(fd); return xs_blade(-1); }
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

    size_t len = strlen(args[2].scroll);
    ssize_t sent = sendto(fd, args[2].scroll, len, 0,
                          (struct sockaddr*)&addr, sizeof(addr));
    close(fd);
    return xs_blade((int64_t)sent);
}

/* ===== udpRecv(port, maxBytes?) -> scroll ===== */

XsValue xs_net_udpRecv(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    int port = (int)xs_as_spark(args[0]);
    int max_bytes = (argc >= 2) ? (int)xs_as_spark(args[1]) : 4096;

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return xs_abyss();

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd); return xs_abyss();
    }

    char* buf = (char*)malloc(max_bytes + 1);
    ssize_t n = recvfrom(fd, buf, max_bytes, 0, NULL, NULL);
    close(fd);
    if (n <= 0) { free(buf); return xs_abyss(); }
    buf[n] = '\0';
    return xs_scroll(buf);
}

/* ===== urlEncode(str) ===== */

XsValue xs_net_urlEncode(int argc, XsValue* args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_abyss();
    char* s = args[0].scroll;
    size_t slen = strlen(s);
    /* Worst case: every char is encoded */
    char* result = (char*)malloc(slen * 3 + 1);
    char* p = result;
    for (size_t i = 0; i < slen; i++) {
        unsigned char c = (unsigned char)s[i];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            *p++ = c;
        } else if (c == ' ') {
            *p++ = '+';
        } else {
            sprintf(p, "%%%02X", c);
            p += 3;
        }
    }
    *p = '\0';
    return xs_scroll(result);
}

/* ===== urlDecode(str) ===== */

XsValue xs_net_urlDecode(int argc, XsValue* args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_abyss();
    char* s = args[0].scroll;
    size_t slen = strlen(s);
    char* result = (char*)malloc(slen + 1);
    char* p = result;
    for (size_t i = 0; i < slen; i++) {
        if (s[i] == '%' && i + 2 < slen) {
            int hi = 0, lo = 0;
            if (sscanf(s + i + 1, "%1x", &hi) == 1 && sscanf(s + i + 2, "%1x", &lo) == 1) {
                *p++ = (char)((hi << 4) | lo);
                i += 2;
            } else {
                *p++ = s[i];
            }
        } else if (s[i] == '+') {
            *p++ = ' ';
        } else {
            *p++ = s[i];
        }
    }
    *p = '\0';
    return xs_scroll(result);
}

/* ===== Registration ===== */

void xs_net_register(VM* vm) {
    vm_register_native(vm, "Net.httpGet",    xs_net_httpGet);
    vm_register_native(vm, "Net.httpPost",   xs_net_httpPost);
    vm_register_native(vm, "Net.tcpConnect", xs_net_tcpConnect);
    vm_register_native(vm, "Net.tcpListen",  xs_net_tcpListen);
    vm_register_native(vm, "Net.tcpAccept",  xs_net_tcpAccept);
    vm_register_native(vm, "Net.tcpSend",    xs_net_tcpSend);
    vm_register_native(vm, "Net.tcpRecv",    xs_net_tcpRecv);
    vm_register_native(vm, "Net.tcpClose",   xs_net_tcpClose);
    vm_register_native(vm, "Net.udpSend",    xs_net_udpSend);
    vm_register_native(vm, "Net.udpRecv",    xs_net_udpRecv);
    vm_register_native(vm, "Net.urlEncode",  xs_net_urlEncode);
    vm_register_native(vm, "Net.urlDecode",  xs_net_urlDecode);
}
