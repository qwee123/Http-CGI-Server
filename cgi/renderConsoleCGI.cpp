#include <iostream>
#include <unistd.h>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <cstdlib>
#include <memory>
#include <vector>
#define MAX_SERVERS 5
using namespace std;
using namespace boost::asio;

io_service service;
posix::stream_descriptor out(service, ::dup(STDOUT_FILENO));

void parseQuery();

void on_write(boost::system::error_code err, size_t bytes){
    if(err)    cerr << "Error on write!" << endl;
}

int main(int argc, char* const argv[]){

   // cout << "HTTP/1.1 200 OK" << endl;
    cout << "Content-type:text/html" << endl;
    cout << endl;
    cout << "<!DOCTYPE html>" << endl;
    cout << "<html>" << endl;
    cout << "before" << endl;
    parseQuery();
    async_write(out, buffer("</html>"), on_write);

    service.run();
    sleep(2);
    async_write(out, buffer("<script>alert(1);</script>"), on_write);    
    return 0;
}

void parseQuery(){
    std::string query(getenv("QUERY_STRING"));       
    vector<string> v_query;
    v_query.reserve(MAX_SERVERS*3);
    boost::algorithm::split(v_query,query,boost::is_any_of("&"));    

    

}
