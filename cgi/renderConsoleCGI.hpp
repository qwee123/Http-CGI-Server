#ifndef RENDERCONSOLECGI_HPP
#define RENDERCONSOLECGI_HPP

#include <iostream>
#include <unistd.h>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <cstdlib>
#include <memory>
#include <fstream>
#include <vector>
using namespace std;
using namespace boost::asio;

void buildSession();
bool queryIsValid(vector<string>::iterator,
    vector<string>::iterator, vector<string>::iterator);

class RemoteSession{
    private:
        int _sess_num;
        string host;
        unsigned int port;
        ifstream test_case;
        ip::tcp::socket _sock;
        ip::tcp::endpoint ep;
        string res;
        string cmd;

    public:
        RemoteSession(vector<string>::iterator,
              vector<string>::iterator, vector<string>::iterator,int );
        ~RemoteSession();

    private:
        void connectToServer();
        void onConnect(const boost::system::error_code &);
        void onMesgSend(const boost::system::error_code &);
        void onMesgRecv(const boost::system::error_code &);
        void receiveFromServer();
        void sendToServer();
};

vector<unique_ptr<RemoteSession>> sessions;

#endif
