#include "server_client.h"

void handle_exception(int new_fd, int code) {
    char * info;
    if(code == 400) {
        info = (char *)"HTTP/1.1 400 Bad Request\r\n\r\n";
    }
    else{
        info = (char *)"HTTP/1.1 502 Bad Gateway\r\n\r\n";
    }
    std::vector<char> content;
    content.assign(info, info+28);
    send_response(content, new_fd);
}