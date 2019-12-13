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
    render();

    service.run(); 
    return 0;
}

void render(){
    cout << "Content-type:text/html\r\n\r\n";
    cout << "<!DOCTYPE html>" << endl;
    cout << "<html lang=\"en\">" << endl;
    cout << "<head>" << endl;
    renderStyle();
    cout << "</head>" << endl;

    cout << "<body>" << endl;
    buildSession();
    renderSessionTable();
    cout << "</body>" << endl;
    cout << "</html>";
}

void renderStyle(){
    cout << "<meta charset=\"UTF-8\" />" << endl;
    cout << "<link rel=\"stylesheet\" href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css\" " <<
      "integrity=\"sha384-MCw98/SFnGE8fJT3GXwEOngsV7Zt27NXFoaoApmYm81iuXoPkFOJwJ8ERdknLPMO\" crossorigin=\"anonymous\" />" << endl;
    cout << "<link href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\" rel=\"stylesheet\" />" << endl;
    cout << "<link rel=\"icon\" type=\"image/png\" " << 
       "href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\" />" << endl;

    cout << "<style>" << endl;
    cout << "  * {" << endl;
    cout << "   font-family: 'Source Code Pro', monospace;" << endl;
    cout << "    font-size: 1rem !important;" << endl;
    cout << "  }" << endl;
    cout << "  body {" << endl;
    cout << "   background-color: #212529;" << endl;
    cout << "  }" << endl;
    cout << "  pre {" << endl;
    cout << "    color: #cccccc;" << endl;
    cout << "  }" << endl;
    cout << "  b {" << endl;
    cout << "    color: #ffffff;" << endl;
    cout << "  }" << endl;
    cout << " </style>" << endl;
}

void renderSessionTable(){
    cout << "<table class=\"table table-dark table-bordered\">" << endl;
    cout << "<thead>" << endl;
    cout << "<tr>" << endl;

    for(auto itr = sessions.begin();itr != sessions.end(); itr++){
        cout << "<th scope=\"col\">" << (*itr)->getSessionName() << "</th>" << endl;      
    }

    cout << "</thead>" << endl;
    cout << "<tbody>" << endl;
    cout << "</tr>" << endl;

    cout << "<tr>" << endl;
    for(auto itr = sessions.begin();itr != sessions.end(); itr++){
        cout << "<td><pre id=\"" << (*itr)->getSessionID() << "\" class=\"mb-0\"></pre></td>" << endl;
    }
    cout << "</tr>" << endl;
    
    cout << "</tbody>" << endl;
    cout << "</table>" << endl;
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

    res.clear();
    cmd.clear();
    cerr << "Session closed!" << endl;
}

string RemoteSession::getSessionName(){
    string name = host + ":" + to_string(port);
    return name;
}

string RemoteSession::getSessionID(){
    string SID = "s" + to_string(_sess_num);
    return SID;
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

void RemoteSession::escapeResHTML(){
    boost::algorithm::replace_all(res,"\r\n","<br>");
    boost::algorithm::replace_all(res,"\n","<br>");
    boost::algorithm::replace_all(res,"\r","<br>");
    boost::algorithm::replace_all(res,"\'","\\'");
}

void RemoteSession::escapeCmdHTML(){
    boost::algorithm::replace_all(cmd,"\n","");
    boost::algorithm::replace_all(cmd,"\r","");
    boost::algorithm::replace_all(cmd,"\'","\\'");
}

void RemoteSession::onMesgRecv(const boost::system::error_code& ec){
    if(ec) {
        cerr << "Error: Fail to recv message" << endl; 
        return ;
    }

    escapeResHTML();//boost's find_first cannot find '\0' correctly...
    auto len = strlen(res.c_str());

    //Memory corruption will happen after the destructor is called if i try to resize,substr res here...    
    string trim_res; 
    trim_res.assign(res,0,len);

    string update_script = "<script>document.getElementById('" + getSessionID() + "').innerHTML += '" 
                      + trim_res + "';</script>\n";
    trim_res.clear();

    async_write(out,buffer(update_script),on_output);
 
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
    
    escapeCmdHTML();
    //auto len = strlen(cmd.c_str());
    //string trim_cmd;
    //trim_cmd.assign(cmd,0,len);

    string update_script = "<script>document.getElementById('" + getSessionID() + "').innerHTML += '<b>"
                      + cmd + "<br></b>';</script>\n";
    //trim_cmd.clear();
    async_write(out,buffer(update_script),on_output);

    sleep(0.5);
    cmd.append("\n");
    _sock.async_send(buffer(cmd),boost::bind(&RemoteSession::onMesgSend,
                this, boost::asio::placeholders::error));

}
