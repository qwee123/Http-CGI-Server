#include "renderConsoleCGI.hpp"
#define MAX_SERVERS 5
#define QUERY_TUPLE 3

io_service service;
posix::stream_descriptor out(service, ::dup(STDOUT_FILENO));

void on_output(const boost::system::error_code &ec, size_t bytes){
    if (ec)   cerr << "Error on output result" << endl;

}

int main(int argc, char* const argv[]){

   // cout << "HTTP/1.1 200 OK" << endl;
    cout << "Content-type:text/html" << endl;
    cout << endl;
    cout << "<!DOCTYPE html>" << endl;
    cout << "<html>" << endl;
    cout << "before" << endl;
    cout << "</html>\n" << endl;

    
    buildSession();

    service.run(); 
    return 0;
}

bool queryIsValid(vector<string>::iterator itr_h,
    vector<string>::iterator itr_p, vector<string>::iterator itr_tc){

    if(boost::algorithm::ends_with(*itr_h,"=")){
        cerr << "Invalid query value(host): " << *itr_h << endl;
        return false;
    }

    if(boost::algorithm::ends_with(*itr_p,"=")){
        cerr << "Invalid query value(port): " << *itr_p << endl;
        return false;
    }

    if(boost::algorithm::ends_with(*itr_tc,"=")){
        cerr << "Invalid query value(tc): " << *itr_tc << endl;
        return false;
    }
    return true;
}

void buildSession(){
    std::string query(getenv("QUERY_STRING"));       
    vector<string> v_query;
    v_query.reserve(MAX_SERVERS*QUERY_TUPLE);
    boost::algorithm::split(v_query,query,boost::is_any_of("&"));    

    int i = 0;
    for(auto itr = v_query.begin();itr != v_query.end();itr += QUERY_TUPLE){
        if(queryIsValid(itr,itr+1,itr+2)){ 
            unique_ptr<RemoteSession> ptr(new RemoteSession(itr,itr+1,itr+2,i));
            sessions.push_back(move(ptr));  
        }
        i++;      
    }
}

RemoteSession::RemoteSession(vector<string>::iterator h,
         vector<string>::iterator p,vector<string>::iterator tc,int sn):_sock(service),_sess_num(sn){ 
 
    auto pos = boost::algorithm::find_first(*h,"=").begin(); 
    host = (*h).substr(pos - (*h).begin() + 1);

    pos = boost::algorithm::find_first(*p,"=").begin();
    port = stoul((*p).substr(pos - (*p).begin() + 1)); //might throw an error!

    pos = boost::algorithm::find_first(*tc,"=").begin();
    string filename = "./test_case/" + (*tc).substr(pos - (*tc).begin() + 1);
    if(!test_case)    test_case.close();
    test_case.open(filename);
    if(!test_case)    cerr << "Error: Cannot open file: " << filename << endl;

    connectToServer();
}

RemoteSession::~RemoteSession(){
    if(test_case)   test_case.close();
    _sock.close();
    cerr << "Session closed!" << endl;
}

void RemoteSession::connectToServer(){
    ip::tcp::resolver resolver(service);
    ip::tcp::resolver::query query(host,to_string(port));
    ip::tcp::resolver::iterator itr = resolver.resolve(query);
    ep = *itr;
    
    _sock.async_connect(ep,boost::bind(&RemoteSession::onConnect, 
           this, boost::asio::placeholders::error));
}

void RemoteSession::onConnect(const boost::system::error_code& ec){
    if(ec) cerr << "Error: Fail to connect!" << endl;

    receiveFromServer();
}

void RemoteSession::onMesgSend(const boost::system::error_code& ec){
    if(ec) cerr << "Error: Fail to send message" << endl;

    receiveFromServer();
}

void RemoteSession::onMesgRecv(const boost::system::error_code& ec){
    if(ec) {
        cerr << "Error: Fail to recv message" << endl; 
        return ;
    }

    async_write(out,buffer(res),on_output);
 
    if(boost::algorithm::find_last(res,"% ").empty()){ 
        receiveFromServer();
    }
    else{
        sendToServer();
    }
}

void RemoteSession::receiveFromServer(){
    res.clear();
    res.resize(1024);
    _sock.async_receive(buffer(res),boost::bind(&RemoteSession::onMesgRecv,
             this, boost::asio::placeholders::error));
}

void RemoteSession::sendToServer(){
    cmd.clear();
    cmd.resize(15000);  
 
    getline(test_case,cmd);
    cmd.append("\n");
    async_write(out,buffer(cmd),on_output);

    sleep(1);
    _sock.async_send(buffer(cmd),boost::bind(&RemoteSession::onMesgSend,
                this, boost::asio::placeholders::error));

}
