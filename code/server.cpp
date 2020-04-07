// #include <stdio.h>
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <netdb.h>
// #include <arpa/inet.h>
// #include <netinet/in.h>
// #include <iostream>
// #include <cstring>
// #include <unistd.h>

// int main(int argc, char *argv[])
// {
//     struct addrinfo hints, *res, *p;
//     int status;
//     struct sockaddr_storage their_addr;
//     socklen_t addr_size;
//     char ipstr[INET6_ADDRSTRLEN];

//     memset(&hints, 0, sizeof hints);
//     hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
//     hints.ai_socktype = SOCK_STREAM;
//     // hints.ai_flags = AI_PASSIVE;

//     if ((status = getaddrinfo("vcm-12231.vm.duke.edu", "12345", &hints, &res)) != 0) {
//         fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
//         return 2;
//     }

//     int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
//     std::cout << "sockfd:" << sockfd << std::endl;
//     bind(sockfd, res->ai_addr, res->ai_addrlen);
//     status = listen(sockfd, 100);
//     std::cout << "listen status: " << status << std::endl;

//     addr_size = sizeof their_addr;
//     int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
//     std::cout << "accepted" << std::endl;

//     char buf[1024];
//     int len_recv = recv(new_fd, buf, 1024, 0);
//     // string s = buf;
//     std::cout << buf << std::endl;
//     std::cout << len_recv << std::endl;

//     close(new_fd);
//     close(sockfd);
    // free(buf);
    // printf("IP addresses for %s:\n\n", argv[1]);

    // for(p = res;p != NULL; p = p->ai_next) {
    //     void *addr;
    //     const char *ipver;

    //     // get the pointer to the address itself,
    //     // different fields in IPv4 and IPv6:
    //     if (p->ai_family == AF_INET) { // IPv4
    //         struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
    //         addr = &(ipv4->sin_addr);
    //         ipver = "IPv4";
    //     } else { // IPv6
    //         struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
    //         addr = &(ipv6->sin6_addr);
    //         ipver = "IPv6";
    //     }

    //     // convert the IP to a string and print it:
    //     inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
    //     printf("  %s: %s\n", ipver, ipstr);
    // }

//     freeaddrinfo(res); // free the linked list

//     return 0;
// }

// #include "server_client.h"

// int main() {
//     try{
//         Server server = Server();
//         server.receive();
//         std::string package = server.get_package();
//         std::cout << package << std::endl;
//     }
//     catch(int e) {
//         if(e == 2) {
//             std::cout << "Get address error" << std::endl;
//         }
//     }
// }

