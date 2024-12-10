#pragma once

#include "json/json.hpp"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

using json = nlohmann::json;
extern bool AUTH_SENT;
extern vector<string> SUPPORTED_CURRENCIES;

class jsonrpc : public json {
    public:
        jsonrpc(){
            (*this)["jsonrpc"] = "2.0",

            srand( time(NULL) );
            long number = rand();
            (*this)["id"] = number;
        }
        
        jsonrpc(const string& method){
            (*this)["jsonrpc"] = "2.0",
            (*this)["method"] = method;
            srand( time(NULL) );
            long number = rand();
            (*this)["id"] = number;
        }
};

namespace api {

    string process(const string &input);

    string authorize(const string &cmd);

    string sell(const string &input);

    string buy(const string &input);

    string get_open_orders(const string &input);

    string modify(const string &input);

    string cancel(const string &input);

    string cancel_all(const string &input);
}