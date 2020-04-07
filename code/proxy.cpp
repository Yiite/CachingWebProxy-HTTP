#include <vector>
#include <thread>
#include <functional>
#include "handle_exception.h"
std::mutex mtx;


void process(int new_fd, string client_ip, LRU_Cache & cache) {
// void process(int new_fd) {
    try {
        //cout << "in thread " << std::this_thread::get_id() << endl;
        Http request(true);
        request.header = receive_header(new_fd);
        string request_generated_time = compute_now_time_string();
        //cout << "Request header ====================" << request.header.size() << endl;
        if(request.header.size() == 0) {
            close(new_fd);
            return;
        }
        std::string request_header_string(request.header.begin(), request.header.end());
        request.http_parse_header(request_header_string); // header invalid should return bad gate way response
        std::string URI = request.header_dict["URI"];
        std::hash<std::string> str_hash;
        string request_id = std::to_string(str_hash(URI));
        Log mylog(request_id);
        Client client = Client(request.header_dict["Host"], request.header_dict["Port"]);
        string request_start_line = "\"" + request.header_dict["Method"] + " " + URI + " " + request.header_dict["Version"] + "\"";
        string loginfo = request_start_line + " from " + client_ip + " @ " + request_generated_time; 
        mylog.write_log(loginfo); 
        int request_created_tm;
        if (request.header_dict["Method"] == "GET"){
            if (cache.is_cached(URI)){
              //  cout << "find URI is ==================" << URI << endl;
                std::lock_guard<std::mutex> lck (mtx);
                Http response = cache.get(request, client, mylog);
                vector<char> header_body_content = response.combine_header_body();
                int i = send_response(header_body_content, new_fd);
                std::string respondinfo = "Responding " + response.header_dict["Version"] + " " + response.header_dict["Status"] +  " " + response.header_dict["Status_Info"];
                mylog.write_log(respondinfo);
                if (i == 1){
                    close(new_fd);
                }
                //cout << "returning here 1" << endl;
                return; 
           }
            else{
                mylog.write_log("not in cache");
                client.send_request(request_header_string);
                string requestinfo = request_start_line + " from " + URI;
                mylog.write_log("Requesting " + requestinfo);
                request_created_tm = compute_now_time();
            }
        }
        else if (request.header_dict["Method"] == "CONNECT"){
            vector<char> connection_confirm;
            char * info = (char *)"HTTP/1.1 200 Connection established\r\n\r\n";
            connection_confirm.assign(info, info+39);
            string connection_info(connection_confirm.begin(), connection_confirm.end());
            string respondinfo = "Responding \"" + connection_info + "\"";
            mylog.write_log(respondinfo);
            int i = send_response(connection_confirm,new_fd);
            if (i == -1) return;
            connect_receive_and_send(client, new_fd);
            close(new_fd);
            mylog.write_log("Tunnel closed");
            //cout << "returning here 2" << endl;
            return;
        }
        else if (request.header_dict["Method"] == "POST"){
            receive_body(request, new_fd);
            vector<char> header_body = request.combine_header_body();
            string package_header_body(header_body.begin(), header_body.end());
            string requestinfo = request_start_line + " from " + URI;
            client.send_request(package_header_body); 
            mylog.write_log("Requesting " + requestinfo);
        }
        //Response
        Http response(false);
        std::vector<char> content = client.get_package(response);
        std::string received_response = "\"" + response.header_dict["Version"] + " " + response.header_dict["Status"] +  " " + response.header_dict["Status_Info"] + "\"";
        std::string receivedinfo = "Received " + received_response + "from" + URI;
        std::string respondinfo = "Responding " + received_response;
        // vector<char> all = response.combine_header_body();
        //cout << "content i am going to send back ---------------- should only header!!!!" << content.data() << endl;
        
        if (content.empty()) {
            char * info = (char *)"HTTP/1.1 502 Bad Gateway\r\n\r\n";
            content.assign(info, info+28);
            string reponsed_generated_time = compute_now_time_string();
            string respondinfo(content.begin(), content.end());
            mylog.write_log("Responding " + respondinfo);
            int i = send_response(content, new_fd);
            if (i == -1){
                return;
            }
            //response is empty need further handling 
        }
        string response_generated_time = compute_now_time_string();
        mylog.write_log(respondinfo);
        int i = send_response(content, new_fd);
        if (i == -1) return;
        if (request.header_dict["Method"] == "GET"){
            std::lock_guard<std::mutex> lck (mtx);
            std::string URI = request.header_dict["URI"];
            cache.store(URI,request, response,request_created_tm, mylog); 
        }
        close(new_fd);
    }

    catch(address_err &e){
        std::cout << e.what() << std::endl;
        handle_exception(new_fd, e.get_code());
    }
    catch(bad_format &e) {
        std::cout << e.what() << std::endl;
        handle_exception(new_fd, e.get_code());
    }

}

int main() {
    Server server = Server();
    LRU_Cache server_cache(128);
    cout << "Proxy up!" << endl;
    while (1) {
        // accept
      //  std::cout << "waiting new request..." << std::endl;
        int new_fd = server.server_accept();
        string ip_add = server.get_client_ip();
        // cout << new_fd << endl;
        // thread (handle(newfd))
        std::thread t(process, new_fd, ip_add, std::ref(server_cache));
        t.detach();
        // process(new_fd, server_cache);
        // close(new_fd);
    }
    
}