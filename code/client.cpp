// #include <cstdio>
// #include <iostream>
// #include <string.h>
// #include <sys/types.h>
// #include <unistd.h>
// #include <sys/socket.h>
// #include <netdb.h>
// #include <arpa/inet.h>
// #include <netinet/in.h>

// using namespace std;

// int main(int argc, char *argv[])
// {
//     struct addrinfo hints, *res, *p;
//     int status;
//     struct sockaddr_storage their_addr;
//     socklen_t addr_size;
//     char ipstr[INET6_ADDRSTRLEN];

//     cout << "first" << endl;

//     memset(&hints, 0, sizeof hints);
//     hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
//     hints.ai_socktype = SOCK_STREAM;
    

//     cout << "second" << endl;

//     if ((status = getaddrinfo("vcm-12231.vm.duke.edu", "12345", &hints, &res)) != 0) {
//         fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
//         return 2;
//     }

//     cout << "third" << endl;

//     int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
//     status = connect(sockfd, res->ai_addr, res->ai_addrlen);

//     cout << "after connect" << status << endl;

//     string s = "Hello World";

//     int bytes_sent = send(sockfd, s.c_str(), s.size(), 0);
//     cout << "sent" << endl;
//     // std::cout << "sockfd:" << sockfd << std::endl;

//     // char buf[1024];

//     // int size = 1024;
//     // // int sum = 0;
//     // // while (sum < size) {
//     // int recv_len = recv(sockfd, buf, 1024, 0);
//     // //     cout << buf;
//     // //     sum += this_time;
//     // // }

//     // cout << buf << endl;

//     freeaddrinfo(res); // free the linked list
//     close(sockfd);
//     return 0;
// }

#include "server_client.h"

int main() {
    try {
        // client Client = client("vcm-12231.vm.duke.edu", "12345", "hello,wo");
        Client client("www.example.com", "80", "GET /example.html HTTP/1.1\r\nHost: www.example.com\r\n\r\n");
        client.send_request();
        client.receive();
        std::string package = Client.get_package();
        if (package.size() == 0) {
            std::cout << "No message received." << std::endl;
        }
        else {
            std::cout << "Received package is: " << package;
        }
    }

    catch(int e) {
        if(e == 2) {
            std::cout << "Get address error." << std::endl;
        }

        if (e == 3) {
            std::cout << "Connection error." << std::endl;
        }
    }

}
