#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <exception>
#include <vector>
#include "parser.h"
#include <algorithm> 
#include <time.h> 
#include <fstream>   
#include <mutex> 
std::mutex logmtx;
using namespace std;
class Log{
private:
    string requestid;
    string filename = "/var/log/erss/proxy.log"; 
public:
    Log(string id):requestid(id){}; 
    void write_log(string info){
        std::lock_guard<std::mutex> lck (logmtx);
        using namespace std;
        ofstream logfile;
        logfile.open(filename, std::ios_base::app);
        if (logfile.is_open()){
            logfile << requestid << ": " << info << endl; 
        }  
        logfile.close();
    }
};

int compute_now_time(){
    time_t now ; 
    struct tm * nowtime; 
    time(&now);
    nowtime = localtime(&now);
    nowtime -> tm_hour += 5;
    time_t nowtime_sec = mktime(nowtime);
    return nowtime_sec;
}

string compute_now_time_string(){
    time_t now ; 
    struct tm * nowtime; 
    time(&now);
    nowtime = localtime(&now);
    nowtime -> tm_hour += 5;
    return string(asctime(nowtime));
}


// vector<char> receive_header(int new_fd) {
//     char buf[16384];
//     // int num_special = 0;
//     vector<char> package_vec;

//     // while(1){
//         memset(buf,0,sizeof(char));
//         std::cout << "before recv!!!" << std::endl;
//         int len_recv = recv(new_fd, buf, 16384, 0);
//         // std::cout << "len is: " << len_recv << endl; 
//         for(int i = 0; i < len_recv; ++i) {
//             package_vec.push_back(buf[i]);
//         }
//         if(len_recv == 0) {
//             std::cout << "when len == 0, package_vec length is: " << package_vec.size() << std::endl;
//         }

//         // if(buf[0] == '\r' || buf[0] == '\n') {
//         //     num_special += 1;
//         // }
//         // else {
//         //     num_special = 0;
//         // }
//         // if(num_special == 4) {
//         //     break;
//         // }
//     // }
//     return package_vec;
// }

vector<char> receive_header(int new_fd) {
    char buf[1];
    int num_special = 0;
    vector<char> package_vec;
    while(1){
        memset(buf,0,sizeof(char));
        int len_recv = recv(new_fd, buf, 1, 0);
        if (len_recv <= 0){
            break;
        };
        package_vec.push_back(buf[0]);
        if(buf[0] == '\r' || buf[0] == '\n') {
            num_special += 1;
        }
        else {
            num_special = 0;
        }
        if(num_special == 4) {
            break;
        }
    }
    return package_vec;
}

void receive_body(Http &request_response, int sockfd){
        // Parser parser = Parser();
        // //cout << "-------------------------------The header is: " << package_vec.data() << endl;
        // string encoding_type = parser.get_encoding_type(string(header_package.data()));
        if (!request_response.is_request){
            string status_code = request_response.header_dict["Status"];
            char first_digit = status_code[0];
            if (first_digit == '5' || status_code == "204" || status_code == "304"){
                return;
            }
        }
        
        if(request_response.header_dict.find("Transfer-Encoding") == request_response.header_dict.end()) {
            // std::cout << "0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0" << std::endl;
            unordered_map<string, string>::iterator find_content_len = request_response.header_dict.find("Content-Length");
            if(find_content_len == request_response.header_dict.end()) {
                throw bad_format("Response has no body", 502);
            }
            const int content_length = stoi(request_response.header_dict["Content-Length"]);
            char bodybuff[1] = {0};
            for (int i = 0; i < content_length; ++i){
                memset(bodybuff, 0, sizeof(char));
                // cout << "Response before receive body!!!" << endl;
                int len_recv = recv(sockfd, bodybuff, 1, 0); 
                request_response.body.push_back(bodybuff[0]);
            }
        }
        else {
                // if(encoding_type == "chunked") {
                char buf[65536];
                int len_recv = recv(sockfd, buf, 65536, 0);
                for(int i = 0; i < len_recv; i++) {
                    request_response.body.push_back(buf[i]);
                }
                // request_response.body = receive_header(sockfd);  //indeed receive chunk ;
                // cout << "header!!!!!!!!!!!!!!!!"<< request_response.header.data() << endl;
                if (request_response.body.empty()){
                   throw bad_format("Chunked: post has no body", 502);
                }
                // }
                // else if(encoding_type == "deflate") {
                // }
        }
}

int send_response(vector<char> &content, int new_fd) {
    int want_send = content.size();
    int actual_sent = 0;
    while(actual_sent != want_send) {
        int part_sent = send(new_fd, content.data(), content.size(), MSG_NOSIGNAL);
        if (part_sent == -1) return -1;
        actual_sent += part_sent;
    }
    return 1;
}




class Server {
private:
    struct addrinfo *res;
    // std::string package;
    int sockfd;
    // int new_fd;
    string clientIP;

public:
    Server(): sockfd(-1) {
        struct addrinfo hints;
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        int status = 0;
        if ((status = getaddrinfo(NULL, "12345", &hints, &res)) != 0) {
            perror("Get address error");
        }

        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        int on = 1;
        status = setsockopt(sockfd, SOL_SOCKET,SO_REUSEADDR, &on, sizeof(on));
        if (sockfd == -1) {
            std::cout << "socket allocation failed!" << std::endl;
        }
       // std::cout << "socket built! Using file descriptor " << sockfd << "\n";

        status = bind(sockfd, res->ai_addr, res->ai_addrlen);
        if (status == -1) {
            std::cout << "bind failed!" << std::endl;
        }    
        status = listen(sockfd, 200);
        if (status == -1) {
            perror("listen_failed");
        }  
      //  std::cout << "listening..." << std::endl;
    }

    int server_accept() {
        struct sockaddr_storage their_addr;
        socklen_t addr_size = sizeof their_addr;
        int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
        if (new_fd == -1) perror("accept failed");
        struct sockaddr_in *sin = (struct sockaddr_in *) &their_addr;
        unsigned char *ip = (unsigned char *) &sin -> sin_addr.s_addr;
        char ipstr[20];
        int n = sprintf(ipstr, "%d.%d.%d.%d", ip[0], ip[1], ip[2],ip[3]);
        clientIP.assign((const char *)ipstr); 
        return new_fd;
    }

    string get_client_ip(){
        return clientIP;
    }

    ~Server() {
        if(sockfd != -1) {
            close(sockfd);
        }
        freeaddrinfo(res);
    }
};

class Client {
private:
    struct addrinfo *res;
    std::string package; // string only contains header, used for parse 
    int sockfd;
    std::string ip;
    std::string port;
    // std::string content;  // request send to host server 

public:
    Client(std::string ip_address, std::string port_number): package(""), sockfd(-1), ip(ip_address), port(port_number) {
        struct addrinfo hints;
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;
        int status = 0;
        if ((status = getaddrinfo(ip.c_str(), port.c_str(), &hints, &res)) != 0) {
            if (status == EAI_SYSTEM) {
                perror("getaddrinfo");
            } 
            else {
                fprintf(stderr, "error in getaddrinfo: %s\n", gai_strerror(status));
            }
            string erro_mess = "Name or Service Not Known:" + ip_address + ":" + port_number;
            throw bad_format(erro_mess.c_str(), 502);
        }

        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd == -1) {
            std::cout << "socket allocation failed!" << std::endl;
        }
       // std::cout << "socket built! Using file descriptor " << sockfd << "\n";  
        status = connect(sockfd, res->ai_addr, res->ai_addrlen);
    }

    int send_request(string &content) {
        int want_send = content.size();
        int actual_sent = 0;
        while(actual_sent != want_send) {
            int part_sent = send(sockfd, content.c_str(), content.size(), MSG_NOSIGNAL);
            if (part_sent == -1) return -1;
            actual_sent += part_sent;
        }
        return 1; 
    }
    void fill_response(Http &response){
        response.header = receive_header(sockfd);
        if (!response.header.empty()){
            std::string response_header_string(response.header.begin(), response.header.end());
            response.http_parse_header(response_header_string);
            receive_body(response, sockfd);
        }
    }
    
    std::vector<char> get_package(Http &response) {
        fill_response(response);
        return response.combine_header_body();
    }

    int get_client_fd(){
        return sockfd;
    }

    ~Client() {
        if(sockfd != -1) {
            close(sockfd);
        }
        freeaddrinfo(res);
    }
};


void connect_receive_and_send(Client &client, int broswer_fd){
    int len_recv = 1;
    int client_fd = client.get_client_fd();
    struct timeval interval;
    fd_set readfds, testfds;
    interval.tv_sec = 90;
    interval.tv_usec = 0; 
    FD_ZERO(&readfds);
    FD_SET(broswer_fd, &readfds);
    FD_SET(client_fd, &readfds);
    int maxfd = max(client_fd, broswer_fd); 
    while (true){
        testfds = readfds;
        int status = select(maxfd + 1, &testfds, NULL, NULL, &interval);
        if (status == -1) {
            std::cerr << "select failed\n";
            break;
        }
        if (status == 0) break;
        // cout << "status" << status << "\n"; 
        if (FD_ISSET(broswer_fd, &testfds)){
            string buff(600000, ' ');
           //std::cout << "receiving from browser..." << std::endl;
            int len_recv = recv(broswer_fd, &buff[0], 600000,MSG_DONTWAIT);
            if(len_recv == 0) return;
            if (len_recv < 0){
                perror("whats wrong with receive");
                return;
            }
            // std::cout << "after connections recv from browser" << std::endl;
            buff.resize(len_recv);
            int send_status = client.send_request(buff);
            if (send_status == -1) return ;
        }
        if (FD_ISSET(client_fd, &testfds)){
            //vector<char> buff(500000, 0);
            vector<char> buff(600000);
            // cout << "here now" << endl;
           // std::cout << "receciving from server..." << std::endl;
            int len_recv = recv(client_fd, &buff.data()[0], 600000, MSG_DONTWAIT);
            // std::cout << "after connections recv from server" << std::endl;
            if(len_recv == 0) return;
            if (len_recv < 0){
                perror("whats wrong with receive");
                return;
            }
            // cout << "received_length" << len_recv << endl;
            buff.resize(len_recv);
            int status = send_response(buff, broswer_fd);
            if (status == -1) return;
        }
    }
} 



class CacheNode{
    public:
        Http request; 
        Http response;
        CacheNode * next; 
        CacheNode * prev; 
        int request_time;
        int response_time; 
    public:
        CacheNode(Http RESPONSE, Http REQUEST,CacheNode * NEXT, CacheNode * PREV, int request_created_time):request(REQUEST), response(RESPONSE),next(NEXT), prev(PREV),request_time(request_created_time){
            response_time = compute_now_time();
        };
};


class Cachelinkedlist{
    public:
        CacheNode * head;
        CacheNode * tail;
    public:
        Cachelinkedlist():head(NULL), tail(NULL){}
        void add_back(Http &Request,Http &Response, int request_created_time){
            CacheNode * new_node = new CacheNode(Request, Response, NULL, tail, request_created_time);
            if (head == NULL){
                head = new_node; 
                tail = head; 
            }
            else{
                tail -> next = new_node; 
                tail = new_node;
            }
        }

        void delete_head(){
            if (head == NULL){
                return; 
            }
            CacheNode * temp = head -> next;
            if (temp == NULL){
                delete head; 
                head = NULL;
                tail = NULL; 
            }
            temp -> prev = NULL;
            delete head; 
            head = temp; 
        }

        void move_to_end(CacheNode * Node){
            if (tail == Node) return; 
            if (head == Node){
                tail -> next = Node;
                Node -> prev = tail; 
                Node -> next -> prev = NULL;
                head = Node -> next; 
                Node -> next = NULL;
                tail = Node; 
                return; 
            }
            Node -> prev -> next = Node -> next;
            Node -> next -> prev = Node -> prev;
            Node -> prev = tail; 
            Node -> next = NULL;
            tail -> next = Node; 
            tail = Node; 
        }
};






// }
vector<string> query(Http  *r, string query_key){
    vector<string> r_query; 
    auto r_it = (r -> header_dict).find(query_key);
    if (r_it != (r -> header_dict).end()){
        Parser parser;
        r_query = parser.sep_str(r_it -> second, ", "); 
    }
    return r_query; 
}

class LRU_Cache{
    private:
        unordered_map<string, CacheNode *> cache_query_map;
        int max_size ;
        Cachelinkedlist cachelist; 
    public:
        LRU_Cache(int capacity):max_size(capacity){};

        Http get(Http &new_request, Client & client, Log & log){
           // cout << "get from cache " << endl;
            string URI = new_request.header_dict["URI"];
            CacheNode * uri_Node = cache_query_map[URI];
            Http * response = &(uri_Node -> response);
            Http * request = &(uri_Node -> request);
            if (!vary_matched(request -> header_dict, new_request.header_dict, response -> header_dict)){
                log.write_log("in cache, requires validation");
                // cout << "vary fields are different need validation: \n " << response -> header.data() << endl;
                *request = new_request; 
                if(Validate(client, uri_Node) == 0){
                    if (response -> header_dict["Status"] != "112"){
                        bool _ = replace_existing_fields("HTTP/1.1 ", "112 Disconnected Operation", response);
                        response -> header_dict["Status"] = "112"; 
                    }
                }
                // cout << "vary fields updated \n" << response -> header.data() << endl;
                cachelist.move_to_end(uri_Node); 
                return uri_Node -> response; 
            }
            int Age = 0;
            vector<string> request_cache_control = query(request, "Cache-Control"); 
            vector<string> response_cache_control = query(response, "Cache-Control");
            if (find(request_cache_control.begin(), request_cache_control.end(), "no-cache") != request_cache_control.end() 
             || find(response_cache_control.begin(), response_cache_control.end(), "no-cache") != response_cache_control.end()){
                log.write_log("in cache, requires validation");
                if(Validate(client, uri_Node) == 0 ){
                    if (response -> header_dict["Status"] != "112"){
                        bool _ = replace_existing_fields("HTTP/1.1 ", "112 Disconnected Operation", response);
                        response -> header_dict["Status"] = "112"; 

                    }
                }
                // cout << "liu le liu le liu le " << endl;
                cachelist.move_to_end(uri_Node); 
                return uri_Node -> response;
            }

            if (!is_fresh(uri_Node, response_cache_control, Age, log)){ // not fresh 
                if (find(response_cache_control.begin(), response_cache_control.end(), "must-revalidate") == response_cache_control.end()
                && find(response_cache_control.begin(), response_cache_control.end(), "proxy-revalidate") == response_cache_control.end())
                {    
                    if (response -> header_dict["Status"] != "110"){
                        bool _ = replace_existing_fields("HTTP/1.1 ", "200 Response is Stale", response);
                        // response -> header_dict["Status"] = "110"; 
                    }
                }
                else{
                    if(Validate(client, uri_Node) == 0){
                        if (response -> header_dict["Status"] != "112"){
                            bool _ = replace_existing_fields("HTTP/1.1 ", "112 Disconnected Operation", response);
                            response -> header_dict["Status"] = "112"; 
                        }
                    }
                }
            }

            cachelist.move_to_end(uri_Node);
            string current_age = to_string(Age);
            // cout << "NOW age" << Age << endl;
            if (!replace_existing_fields("Age: ", current_age, response)){
                // cout << "hererherher " << endl;
                string field_key_value = "Age: " + current_age + "\r\n\r\n"; 
                (response -> header).erase((response -> header).end() - 2, (response -> header).end());
                for (int i = 0; i < field_key_value.size(); ++i) (response -> header).push_back(field_key_value[i]);
                // cout << "response_header_______________- " << response -> header.data() << "pp"  << endl;
            } 
            return uri_Node -> response;
       }


        bool vary_matched(unordered_map<string, string> & originial_request_header, unordered_map<string, string> & new_request_header, unordered_map<string, string> & response_header){
            if (response_header.find("Vary") == response_header.end()){
                return true; 
            }
            else{
                string vary_fields = response_header["Vary"];
                Parser parser;
                vector<string> splited_vary_content = parser.sep_str(vary_fields, ", ");
                for (string ele : splited_vary_content){
                    auto it_o = originial_request_header.find(ele);
                    auto it_n = new_request_header.find(ele);
                    if (it_o != originial_request_header.end() && it_n != new_request_header.end()){
                        if (it_o -> second != it_n -> second){
                            return false;
                        }
                    }
                    else if (it_o == originial_request_header.end() && it_n == new_request_header.end()){
                        continue;
                    }
                    else{
                        return false;
                    }
                }
                return true;
            }
        }

    void find_which_lifetime(int & lifetime, bool & used_heurstic, vector<string> controls, Http * response){
         for (string element: controls){
            int size = element.size();
            if (element.find("S-maxage") != string::npos){
                lifetime = stoi(element.substr(9, size - 9));
                break;
            }
            else if (element.find("max-age") != string::npos){
                lifetime = stoi(element.substr(8, size - 8));
            }
        }
        if (lifetime == -327){
            if (response -> header_dict.find("Expires") != response -> header_dict.end()){
                lifetime = calculate_lifetime("Expires", response);       
            }
        }
        if (lifetime == -327){
            if (response -> header_dict.find("Last-Modified") != response -> header_dict.end()){
                lifetime = calculate_lifetime("Last-Modified", response);
                used_heurstic = true;
            } 
        }
    }
    string calculate_expiration_date(time_t date_sec, int lifetime){
        struct tm * expirationdate; 
        expirationdate -> tm_isdst = -1; 
        time_t expiration_sec = date_sec + lifetime; 
        expirationdate = localtime(&expiration_sec);
        string expiration_date_string = string(asctime(expirationdate));
        return expiration_date_string;
    };

    bool is_fresh(CacheNode * current_node, vector<string> cache_control_values, int &current_age, Log & log){
        Http * response = &(current_node -> response);
        bool used_heurstic = false;
        int lifetime = -327;
        find_which_lifetime(lifetime, used_heurstic, cache_control_values, response);
        if (lifetime == -327) return false; 
        time_t nowtime_sec = compute_now_time();
        const char * Date = (response -> header_dict)["Date"].c_str();
        struct tm date_tm;
        date_tm.tm_isdst = -1; 
        strptime(Date, "%a, %d %h %Y %X", &date_tm);
        time_t date_sec = mktime(&date_tm);
        string expiration_date_string = calculate_expiration_date(date_sec, lifetime); 
        time_t corrected_reveived_age = nowtime_sec - date_sec;
        if (response -> header_dict.find("Age") != response -> header_dict.end()){
            int age_in_field = stoi( response -> header_dict["Age"]);
            corrected_reveived_age = max((int)corrected_reveived_age, age_in_field);
        }
        int response_delay = current_node -> response_time - current_node -> request_time;
        int corrected_initial_age = corrected_reveived_age + response_delay; 
        int resident_time = compute_now_time() - current_node -> response_time; 
        current_age = corrected_initial_age + resident_time; 
      //  cout << "currnet age ++++++++++++" << current_age << endl;
       // cout << "lifetime is +++++++++++++++  " << lifetime << endl;
        if (lifetime > current_age) {
            if (used_heurstic && current_age > 864000){
                bool _ = replace_existing_fields("HTTP/1.1 ", "113 Heursitic Expiration", response);
                response -> header_dict["Status"] = "113";
            }
            log.write_log("in cache, valid");
            return true;
        }
        else{
            string loginfo = "in cache, but expired at " + expiration_date_string;  
            log.write_log(loginfo);
            return false;
        }
    }

    int calculate_lifetime(string field1, Http * response){
            const char * field_time = (response -> header_dict)[field1].c_str();
            const char * Date = (response -> header_dict)["Date"].c_str();
            struct tm field_tm;
            field_tm.tm_isdst = -1;
            struct tm date_tm;
            date_tm.tm_isdst = -1;
            strptime(field_time, "%a, %d %h %Y %X", &field_tm);
            strptime(Date, "%a, %d %h %Y %X", &date_tm);
            time_t field_sec  = mktime(&field_tm);
            time_t date_sec = mktime(&date_tm);
            int lifetime = (int)field_sec - (int)date_sec;   
            if (field1 == "Last-Modified") lifetime *= 0.1;
            return lifetime; 
    }
       
    bool replace_existing_fields(string fields_key, string fields_value, Http * modified_http){
        string request_or_response(modified_http -> header.begin(), modified_http -> header.end());
        size_t fields_start = request_or_response.find(fields_key); 
        if (fields_start != string::npos){
            size_t fields_end = request_or_response.find("\r\n", fields_start);
            string fields_and_content = fields_key + fields_value;
            vector<char> replacement;
            char * replacement_char = (char*)fields_and_content.c_str(); 
            replacement.assign(replacement_char, replacement_char + (int)fields_and_content.size());
            vector<char> new_header; 
            new_header.insert(new_header.end(), (modified_http -> header).begin(), (modified_http -> header).begin() + fields_start);
            new_header.insert(new_header.end(), replacement.begin(), replacement.end());
            new_header.insert(new_header.end(), (modified_http -> header).begin() + fields_end, (modified_http -> header).end());
         //   cout << "checking changed headerrrrrrrrrrrr " << new_header.data() << endl;
            modified_http -> header = new_header; 
         //   cout << "checking after headerrrrrrrrrrrr " << modified_http -> header.data() << endl;
            modified_http -> header.shrink_to_fit();
            return true;
        }
        else{
            return false;
        }
    }

    void add_validator(Http * request, Http* response, string validated_field, string validator_field){
        string field_value = (response -> header_dict)[validated_field];
        string field_key = validator_field;
        if (!replace_existing_fields(field_key, field_value, request)){
            string field_key_value = field_key + field_value + "\r\n\r\n"; 
            (request -> header).erase((request -> header).end() - 2, (request -> header).end());
            for (int i = 0; i < field_key_value.size(); ++i) (request -> header).push_back(field_key_value[i]);
        }
    }
    
    int Validate(Client & client, CacheNode * current_cache_node){
        Http * response = &(current_cache_node -> response);
        Http * request  = &(current_cache_node -> request);
        if ((response -> header_dict).find("Etag") != (response -> header_dict).end()){
            add_validator(request, response, "Etag", "If-Non-Match: ");
        }
        else if ((response -> header_dict).find("Last-Modified") != (response -> header_dict).end()){
            add_validator(request, response, "Last_Modified", "If-Modified-Since: "); 
        }
        string new_request_header((request -> header).begin(), (request->header).end());
        Http new_response(false);
        int send_status = client.send_request(new_request_header);
        time_t new_request_time = compute_now_time();
        if (send_status == -1) return 0; 
        client.fill_response(new_response);
        if (new_response.header_dict["Status"] != "304"){
            current_cache_node -> request_time = new_request_time;
            current_cache_node -> response_time = compute_now_time();
            current_cache_node -> response = new_response; 
        }
        else{
            if (response -> header_dict["Status"] != "200"){
                bool _ = replace_existing_fields("HTTP/1.1 ", "200 OK", response);
                response -> header_dict["Status"] = "200";
            }
        }
        return 1; 
    }

    void store(string URI,Http Request,  Http Response, int request_created_time, Log &log){
        if (cache_query_map.size() == max_size){
            cache_query_map.erase(cachelist.head -> request.header_dict["URI"]);
            cachelist.delete_head();
        }
        vector<string> request_cache_control = query(&Request, "Cache-Control");
        vector<string> response_cache_control = query(&Response, "Cache-Control");
        if (request_cache_control.empty() && response_cache_control.empty()){
            if (Response.header_dict.find("Expires") == Response.header_dict.end()) {
                log.write_log("not cacheable because the get header does not have cache-control or expire field");
                return;
            }
        }
        else{
            if (find(request_cache_control.begin(),request_cache_control.end(),"no-store") != request_cache_control.end() || find(response_cache_control.begin(),response_cache_control.end(),"no-store") != response_cache_control.end())
                {                   
                    log.write_log("not cacheable because the get header indicates no-store in its field");
                    return; 
                }
        }
       // cout << "stored !!!!!!!!!! \n";
       if (find(request_cache_control.begin(), request_cache_control.end(), "no-cache") != request_cache_control.end() || find(response_cache_control.begin(), response_cache_control.end(), "no-cache") != response_cache_control.end()){
            log.write_log("cached, but requires re-validation");
            cachelist.add_back(Response, Request, request_created_time);
            cache_query_map[URI] = cachelist.tail; 
            return ;
        }
        if (find(response_cache_control.begin(), response_cache_control.end(), "must-revalidate") == response_cache_control.end() && find(response_cache_control.begin(), response_cache_control.end(), "proxy-revalidate") == response_cache_control.end()){
            log.write_log("cached, but requires revalidation"); 
            cachelist.add_back(Response, Request, request_created_time);
            cache_query_map[URI] = cachelist.tail; 
            return;
        }
        bool used_heurstic = false;
        int lifetime = -327;
        find_which_lifetime(lifetime, used_heurstic, response_cache_control, &Response);
        if (lifetime == -327){
            log.write_log("cached, but requires revalidation"); 
            cachelist.add_back(Response, Request, request_created_time);
            cache_query_map[URI] = cachelist.tail; 
            return;
        }
        time_t nowtime_sec = compute_now_time();
        const char * Date = (Response.header_dict)["Date"].c_str();
        struct tm date_tm;
        date_tm.tm_isdst = -1; 
        strptime(Date, "%a, %d %h %Y %X", &date_tm);
        time_t date_sec = mktime(&date_tm);
        string expiration_date_string = calculate_expiration_date(date_sec, lifetime); 
        log.write_log("cached, expires at " + expiration_date_string); 
        cachelist.add_back(Response, Request, request_created_time);
        cache_query_map[URI] = cachelist.tail; 
        return;

    }

    bool is_cached(string URI){
        return cache_query_map.find(URI) != cache_query_map.end(); 
    }


 };






