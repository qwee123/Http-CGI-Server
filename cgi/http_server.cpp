#include <iostream>
#include <stdio.h>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <sys/wait.h>
#include <cstdlib>
#include <memory>
#include <array>
#include <utility>
#include <boost/algorithm/string/predicate.hpp>
using namespace std;
using namespace boost::asio;

io_service global_io_service;

void signalHandler(int signo){
    if(SIGINT == signo){
        cout << "Exit" << endl;
        exit(0);
    }
    else if(SIGCHLD == signo){
        int pid,status;
        cout << "Terminate Child!" << endl;
        while((pid = waitpid(-1,&status,WNOHANG) > 0))   ;
    }
}

class HttpServer {
    private:
        ip::tcp::acceptor _acc;
        ip::tcp::socket _sock;
        boost::asio::streambuf buf;        
   
    public:
        HttpServer(short port)
            : _acc(global_io_service, ip::tcp::endpoint(ip::tcp::v4(), port)),
              _sock(global_io_service) {
            do_accept();
        }

    private:
        void do_accept(){
            _acc.async_accept(_sock, boost::bind(&HttpServer::handle_accept, 
                          this, boost::asio::placeholders::error));
        }
       
        void handle_accept(const boost::system::error_code& err){
            if(!err){
                boost::asio::streambuf::mutable_buffers_type header = buf.prepare(1024);
                _sock.async_receive(header,boost::bind(&HttpServer::handle_header_recv,
                          this ,boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
            } 
            else    cerr << "Erron on Accept!" << endl;  
        }

        void handle_header_recv(const boost::system::error_code& err,size_t n){
            if(!err){
                buf.commit(n);
                parseHttpHeader();
                buf.consume(1024);

                react();
            }
            else    cerr << "Erron on receive http header!" << endl;
        }

        void react(){
            char *var = getenv("REQUEST_METHOD");
           
            if(strcmp(var,"GET")){
                cerr << "Error: Unallow request method: " << var << endl;
                return;
            }            

            sendCGI();
        }

        void parseHttpHeader(){
            std::istream is(&buf);
            std::string query;
            is >> query;
            setenv("REQUEST_METHOD",query.c_str(),1); 
            
            is >> query;
            setenv("REQUEST_URI",query.c_str(),1);
            auto s = boost::algorithm::find_first(query,"?");
            if(s.empty()) //no question mark found
                setenv("QUERY_STRING"," ", 1);
            else
                setenv("QUERY_STRING",query.substr(s.begin() - query.begin() + 1).c_str(), 1);
 
            is >> query;
            setenv("SERVER_PROTOCOL",query.c_str(),1);
            
            is >> query;
            is >> query;
            setenv("HTTP_HOST",query.c_str(),1);

            ip::tcp::endpoint ep = _sock.remote_endpoint();
            std::string remote_addr = ep.address().to_string();
            setenv("REMOTE_ADDR", remote_addr.c_str(),1);

            unsigned short remote_port = ep.port();
            setenv("REMOTE_PORT",  to_string(remote_port).c_str(),1);

            ep = _sock.local_endpoint();
            std::string local_addr = ep.address().to_string();
            setenv("SERVER_ADDR", local_addr.c_str(), 1);

            unsigned short local_port = ep.port();
            setenv("SERVER_PORT", to_string(local_port).c_str(),1);
        }

        void sendCGI(){
            string status = "HTTP/1.1 200 OK\n";
            _sock.async_send(buffer(status),boost::bind(&HttpServer::onSendCGI,
                        this, boost::asio::placeholders::error));
        }

        void onSendCGI(const boost::system::error_code &err){
            pid_t pid;
            while((pid = fork()) < 0){
                cerr << "Error: Cannot fork.....retry" << endl;
                sleep(1);
            }

            if(pid==0){
                string uri(getenv("REQUEST_URI"));
                auto pos = boost::algorithm::find_first(uri,"?").begin();
                uri = uri.substr(0,(pos - uri.begin()));
                uri = "." + uri;
                cout << "Start " << uri << endl;          
                
                auto sockfd = _sock.lowest_layer().native_handle();
                dup2(sockfd, STDOUT_FILENO);
                close(sockfd);
                
                int stat = execlp(uri.c_str(),uri.c_str(),NULL);
                if(stat < 0){
                    cerr << "Error: Unknown request URI: " << uri << endl;
                    exit(0); //End child process
                }                             
            }
            else{
                _sock.close();
                do_accept();
            }
        }
};

int main(int argc, char* const argv[]){
    if(argc!=2) {
        cerr << "Usage:" << argv[0] << " [port]" << endl;
        return 1;
    }

    signal(SIGCHLD, signalHandler);
    signal(SIGINT, signalHandler);

    unsigned short port = atoi(argv[1]);
    HttpServer server(port);
    global_io_service.run();
}
