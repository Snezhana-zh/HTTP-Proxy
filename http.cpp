#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <cstring>
#include <stdexcept>

#include <llhttp.h>

#include "http.h"

int on_method_complete(llhttp_t *parser) {
    http_request_t *request = static_cast<http_request_t*>(parser->data);
    request->method = llhttp_get_method(parser);
    return 0;
}

int on_url(llhttp_t *parser, const char *url, unsigned long len) {
    http_request_t *request = static_cast<http_request_t*>(parser->data);

    if (request->url) {
        int curr_len = strlen(request->url);
        request->url = (char*)realloc(request->url, curr_len + len + 1);
        memcpy(request->url + curr_len, url, len);
        request->url[curr_len + len] = 0;
    } else {
        request->url = strndup(url, len);
    }

    return 0;
}

int on_request_complete(llhttp_t *parser) {
    http_request_t *request = static_cast<http_request_t*>(parser->data);
    request->done = 1;
    return 0;
}

int on_response_complete(llhttp_t *parser) {
    http_response_t *response = static_cast<http_response_t*>(parser->data);
    response->done = 1;
    return 0;
}

int on_status_complete(llhttp_t *parser) {
    http_response_t *response = static_cast<http_response_t*>(parser->data);
    response->status = llhttp_get_status_code(parser);
    return 0;
}

void http_request_init(http_request_t *request) {
    memset(request, 0, sizeof(http_request_t));

    llhttp_settings_init(&request->settings);
    request->settings.on_message_complete = on_request_complete;
    request->settings.on_url = on_url;
    request->settings.on_method_complete = on_method_complete;

    llhttp_init(&request->parser, HTTP_REQUEST, &request->settings);
    request->parser.data = request;
}

void http_response_init(http_response_t *response) {
    memset(response, 0, sizeof(http_response_t));

    llhttp_settings_init(&response->settings);
    response->settings.on_message_complete = on_response_complete;
    response->settings.on_status_complete = on_status_complete;

    llhttp_init(&response->parser, HTTP_RESPONSE, &response->settings);
    response->parser.data = response;
}

int http_request_parse(http_request_t *request, char *buf, int len) {
    llhttp_errno_t err = llhttp_execute(&request->parser, buf, len);
    if (err != HPE_OK) {
        return -1;
    }

    return 0;
}

int http_response_parse(http_response_t *response, char *buf, int len) {
    llhttp_errno_t err = llhttp_execute(&response->parser, buf, len);
    if (err != HPE_OK) {
        return -1;
    }

    return 0;
}

char* http_host_from_url(char* url) {
    if (!url) {
        return nullptr;
    }

    if (strncmp(url, "http://", 7) != 0) {
        return nullptr;
    }

    const char* host_start = url + 7;

    const char* host_end = strchr(host_start, '/');

    if (!host_end) {
        host_end = host_start + strlen(host_start);
    }

    size_t host_length = host_end - host_start;

    char* host = (char*)malloc(host_length + 1);
    if (!host) {
        return nullptr;
    }

    strncpy(host, host_start, host_length);
    host[host_length] = '\0';

    return host;
}
