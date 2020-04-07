#include <exception>

using namespace std;

class bad_format: public exception {
private:
    const char * erro_msg;
    int code;

public:
    bad_format(const char * e_m, int c): erro_msg(e_m), code(c) {}
    
    virtual const char* what() const throw() {
         return erro_msg;
     }
    
    int get_code() {
        return code;
    }
};

class address_err: public bad_format{
public:
    address_err(const char * e_m, int c): bad_format(e_m, c){}
};

