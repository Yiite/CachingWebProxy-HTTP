#include <string>
#include <unordered_map>
#include "my_exceptions.h"

using namespace std;

class Parser{
public:
    // string get_encoding_type(string header) {
    //     size_t pos_content = header.find("Transfer-Encoding");
    //     if (pos_content == string::npos){
    //         return "";
    //     }
    //     size_t pos_r_n = header.find("\r\n", pos_content);
    //     size_t num_start_pos = pos_content + 19;
    //     string encoding_type = header.substr(num_start_pos, pos_r_n - num_start_pos);
    //     cout << "encoding_type___^^^^^^^^^^^ " << encoding_type << endl;
    //     return encoding_type;
    // }

    // int get_content_length(string header){
    //     size_t pos_content = header.find("Content-Length");
    //     if (pos_content == string::npos){
    //         ;
    //     }
    //     size_t pos_r_n = header.find("\r\n", pos_content);
    //     size_t num_start_pos = pos_content + 16;
    //     string content_length = header.substr(num_start_pos, pos_r_n - num_start_pos);
    //     return stoi(content_length);
    // }

    vector<string> sep_str(string & str, string sep) {
        vector<string> result;
        size_t start = 0;
        size_t pos_spc = str.find(sep);
        while(pos_spc != string::npos) {
            string sub_str = str.substr(start, pos_spc - start);
            result.push_back(sub_str);
            start = pos_spc + sep.size();
            pos_spc = str.find(sep, start);
        }
        size_t rn_pos = str.find("\r\n");
        result.push_back(str.substr(start, rn_pos - start));

        return result;
    }

    void parse_startline(string &startline, unordered_map <string, string> &header_dict, bool is_request) {
        vector<string> parse_result = sep_str(startline, " ");

        if(is_request) {
            if(parse_result.size() != 3) {
                throw bad_format("Reqest startline is invalid", 400);
        }
            header_dict["Method"] = parse_result[0];
            if(header_dict["Method"] != "GET" && header_dict["Method"] != "POST" && header_dict["Method"] != "CONNECT") {
                throw bad_format("Bad request method: ", 400);
            } 
            header_dict["URI"] = parse_result[1];
            header_dict["Version"] = parse_result[2];
        }
        else{
            header_dict["Version"] = parse_result[0];
            header_dict["Status"] = parse_result[1];
            string status_info ;
            for (int i = 2; i < parse_result.size(); ++i) {
                status_info += parse_result[i];
                status_info += " "; 
            }
            header_dict["Status_Info"] = status_info;
        } 

    }

    void parse_header_fields(string &header_fields, unordered_map <string, string> &header_dict) {
        size_t start = 0;
        size_t pos_colon = header_fields.find(":");
        while(pos_colon != string::npos){
            string key =  header_fields.substr(start, pos_colon - start);
            start = pos_colon + 2;
            size_t pos_rn = header_fields.find("\r\n", start);
            string value = header_fields.substr(start, pos_rn - start);
            start = pos_rn + 2;
            pos_colon = header_fields.find(":", start);
            header_dict[key] = value;
        }
    }

    void parse_host_port(unordered_map <string, string> &header_dict) {
        string host_string = header_dict["Host"];
        size_t pos_colon = host_string.find(":");
        if(pos_colon == string::npos) {
            if(header_dict["Method"] == "CONNECT"){
                header_dict["Port"] = "443";
            }
            else{
                header_dict["Port"] = "80";
            }
            return;
        }
        else{
            header_dict["Host"] = host_string.substr(0, pos_colon);
            header_dict["Port"] = host_string.substr(pos_colon + 1, host_string.size() - (pos_colon + 1));
        }
    }

    void parse_header(unordered_map<string, string> &header_dict, string &header_string, bool is_request) {
        size_t pos_fr_rn = header_string.find("\r\n");
        if(pos_fr_rn == string::npos) {
            if(is_request == true){
                throw bad_format("Bad Request Header Format!", 400);
            }
            else{
                throw bad_format("Bad Response Header Format!", 502);
            }
             // 1 throw bad gateway!!
        }
        string startline = header_string.substr(0, pos_fr_rn);
        parse_startline(startline, header_dict, is_request);
        size_t header_fields_start = pos_fr_rn + 2;
        size_t header_fields_end = header_string.find("\r\n\r\n");
        string header_fields = header_string.substr(header_fields_start, header_fields_end - header_fields_start + 2);
        parse_header_fields(header_fields, header_dict);
        if(is_request){
            parse_host_port(header_dict);
        }
    }
    

};



// class Request{
// public:
//     string host;
//     string port;
//     string method; 

// public:
//     Request(string &package){
//         size_t pos_method = package.find(' ');
//         if (pos_method == string::npos){
//             throw 5;
//         }
//         method = package.substr(0, pos_method);
//         if (method != "CONNECT"){
//             port = "80";
//         }
//         else{
//             port = "443";
//         }
//         size_t pos_host = package.find("Host");
//         if (pos_host == string::npos){
//             throw 6;
//         }
//         get_host_name(pos_host, package);
//     }

//     void get_host_name(size_t start_point, string &package){
//         size_t pos_sep = 0; 
//         pos_sep = package.find("\r\n", start_point);
//         int host_name_start = start_point  + 6;
//         int host_name_length = pos_sep - host_name_start;
//         host = package.substr(host_name_start, host_name_length);
//         cout << "original host " << host << '\n'; 
//         size_t colon_pos = host.find(':');
//         if (colon_pos != string::npos){
//             cout << "colon_pos ___" << colon_pos << endl;
//             size_t port_start_pos = colon_pos + 1;
//             size_t port_name_length = pos_sep - port_start_pos;
//             port = host.substr(port_start_pos, port_name_length);
//             host = host.substr(0,colon_pos);
//         }
//     }
// };

class Http{
public:
    bool is_request;
    unordered_map<string, string> header_dict;
    vector<char> header;
    vector<char> body;

public:
    Http(bool is_r): is_request(is_r) {}
    Http & operator = (const Http & rhs){
        if (this != &rhs){
            is_request = rhs.is_request;
            header_dict = rhs.header_dict;
            header = rhs.header;
            header.shrink_to_fit();
            body = rhs.body;
            body.shrink_to_fit(); 
        }
        return *this ; 
    }
    void http_parse_header(string &header_string){
        Parser parser;
        parser.parse_header(header_dict, header_string, is_request);
    }

    vector<char> combine_header_body(){
        vector<char> content;
        content.insert(content.end(), header.begin(), header.end());
        content.insert(content.end(), body.begin(), body.end());
        return content; 
    }

};