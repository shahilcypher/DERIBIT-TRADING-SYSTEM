#include "api/api.h"
#include "utils/utils.h"
#include "json/json.hpp"
#include "authentication/password.h"

#include <iostream>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

using json = nlohmann::json;
bool AUTH_SENT = false;
vector<string> SUPPORTED_CURRENCIES = {"BTC", "ETH", "SOL", "XRP", "MATIC",
                                        "USDC", "USDT", "JPY", "CAD", "AUD", "GBP", 
                                        "EUR", "USD", "CHF", "BRL", "MXN", "COP", 
                                        "CLP", "PEN", "ECS", "ARS",                              
                                    };

// Helper function to validate instrument format (example implementation)
bool api::is_valid_instrument(const string& instrument) {
    // Basic validation: must contain an underscore and end with -PERPETUAL
    return instrument.find('_') != string::npos && 
           instrument.length() > 10 && 
           instrument.substr(instrument.length() - 10) == "-PERPETUAL";
}

string api::process(const string &input) {

    map<string, function<string(string)>> action_map = 
    {
        {"authorize", api::authorize},
        {"sell", api::sell},
        {"buy", api::buy},
        {"get_open_orders", api::get_open_orders},
        {"modify", api::modify},
        {"cancel", api::cancel},

        {"positions", api::view_positions},
        {"orderbook", api::get_orderbook},

        {"subscribe", api::subscribe}
    };

    istringstream s(input.substr(8));
    int id;
    string cmd;
    s >> id >> cmd;

    auto find = action_map.find(cmd);
    if (find == action_map.end()) {
        utils::printerr("ERROR: Unrecognized command. Please enter 'help' to see available commands.\n");
        return "";
    }
    return find->second(input.substr(8));
}

string api::authorize(const string &input) {

    istringstream s(input);
    string auth;
    string id;
    string flag{""};
    string client_id;
    string secret;
    long long tm = utils::time_now();

    s >> id >> auth >> client_id >> secret >> flag;
    string nonce = utils::gen_random(10);

    jsonrpc j;
    j["method"] = "public/auth";
    j["params"] = {{"grant_type", "client_credentials"}, 
    /* 
    NOTE: Changing 'grant_type' to client_signature may help in improving security,
    but will need to use the already provided function utils::get_signature()
    which might invite complexity and errors.
    */
                   {"client_id", client_id},
                   {"client_secret", secret},
                   {"timestamp", tm},
                   {"nonce", nonce},
                   {"scope", "session:name"}
                   };
    if (flag == "-r" && j.dump() != "") AUTH_SENT = true;
    return j.dump();
}

string api::sell(const string &input) {
    string sell;
    string id;
    string instrument;
    string cmd;
    string access_key;
    string order_type;
    string label;
    string frc;
    int contracts{0};
    int amount{0};
    int price{0};

    istringstream s(input);
    s >> id >> sell >> instrument >> label;

    if (Password::password().getAccessToken() == "") {
        utils::printcmd("Enter the access token: ");
        cin >> access_key;
    }
    else {
        access_key = Password::password().getAccessToken();
    }

    utils::printcmd("\nEnter the amount or contracts: ");
    cin >> cmd;
    if (cmd == "contracts") {
        cin >> contracts;
    }
    else if (cmd == "amount") {
        cin >> amount;
    }
    else {
        utils::printerr("\nIncorrect syntax; couldn't place order\n");
        return "";
    }

    utils::printcmd("Enter the order type: ");
    cin >> order_type;
    vector<string> permitted_order_types = {"limit", "stop_limit", "take_limit", "market", "stop_market", "take_market", "market_limit", "trailing_stop"};

    if (!any_of(permitted_order_types.begin(), permitted_order_types.end(), [&](string val){ return val == order_type; }))
    {
        utils::printerr("\nIncorrect syntax; couldn't place order\nOrder type can be only one of \nlimit\nstop_limit\ntake_limit\nmarket\nstop_market\ntake_market\nmarket_limit\ntrailing_stop\n");
        return "";
    }
    else if (order_type == "limit" || order_type == "stop_limit") {
        utils::printcmd("\nEnter the price at which you want to sell: ");
        cin >> price;
    }

    utils::printcmd("Enter the time-in-force value: ");
    cin >> frc;

    vector<string> permitted_tif = {"good_til_cancelled", "good_til_day", "fill_or_kill", "immediate_or_cancel"};
    if (!any_of(permitted_tif.begin(), permitted_tif.end(), [&](string val){ return val == frc; }))
    {
        utils::printerr("\nIncorrect syntax; couldn't place order\nTime-in-force value can be only one of \ngood_til_cancelled\ngood_til_day\nfill_or_kill\nimmediate_or_cancel\n");
        return "";
    } 

    jsonrpc j("private/sell");
    
    j["params"] = {{"instrument_name", instrument},
                   {"access_token", access_key}};
    if (amount) { 
        j["params"]["amount"] = amount;
    }
    if (price) { 
        j["params"]["price"] = price;
    }
    else {
        j["params"]["contracts"] = contracts;
    }
    j["params"]["type"] = order_type;
    j["params"]["label"] = label;
    j["params"]["time_in_force"] = frc;

    return j.dump();
}

string api::buy(const string &input) {
    string buy;
    string id;
    string instrument;
    string cmd;
    string access_key;
    string order_type;
    string label;
    string frc;
    int contracts{0};
    int amount{0};
    int price{0};

    istringstream s(input);
    s >> id >> buy >> instrument >> label;

    if (Password::password().getAccessToken() == "") {
        utils::printcmd("Enter the access token: ");
        cin >> access_key;
    }
    else {
        access_key = Password::password().getAccessToken();
    }

    utils::printcmd("\nEnter 1 for contracts or 2 for amount: ");
    int choice;
    cin >> choice;
    
    if (choice == 1) {
        utils::printcmd("Enter the number of contracts: ");
        cin >> contracts;
    } else if (choice == 2) {
        utils::printcmd("Enter the amount: ");
        cin >> amount;
    } else {
        utils::printerr("\nIncorrect syntax; couldn't place order\n");
        return "";
    }

    utils::printcmd("Enter the order type: ");
    cin >> order_type;
    vector<string> permitted_order_types = {"limit", "stop_limit", "take_limit", "market", "stop_market", "take_market", "market_limit", "trailing_stop"};

    if (!any_of(permitted_order_types.begin(), permitted_order_types.end(), [&](string val){ return val == order_type; }))
    {
        utils::printerr("\nIncorrect syntax; couldn't place order\nOrder type can be only one of \nlimit\nstop_limit\ntake_limit\nmarket\nstop_market\ntake_market\nmarket_limit\ntrailing_stop\n");
        return "";
    }
    else if (order_type == "limit" || order_type == "stop_limit") {
        utils::printcmd("\nEnter the price at which you want to sell: ");
        cin >> price;
    }
    
    utils::printcmd("Enter the time-in-force value: ");
    cin >> frc;
    
    vector<string> permitted_tif = {"good_til_cancelled", "good_til_day", "fill_or_kill", "immediate_or_cancel"};
    if (!any_of(permitted_tif.begin(), permitted_tif.end(), [&](string val){ return val == frc; }))
    {
        utils::printerr("\nIncorrect syntax; couldn't place order\nTime-in-force value can be only one of \ngood_til_cancelled\ngood_til_day\nfill_or_kill\nimmediate_or_cancel\n");
        return "";
    } 

    jsonrpc j("private/buy");

    j["params"] = {{"instrument_name", instrument},
                   {"access_token", access_key}};
    if (amount) { 
        j["params"]["amount"] = amount;
    }
    if (price) { 
        j["params"]["price"] = price;
    }
    else {
        j["params"]["contracts"] = contracts;
    }
    j["params"]["type"] = order_type;
    j["params"]["label"] = label;
    j["params"]["time_in_force"] = frc;

    return j.dump();
}

string api::get_open_orders(const string &input) {
    istringstream is(input);

    int id;
    string cmd;
    string opt1;
    string opt2;
    is >> id >> cmd >> opt1 >> opt2;

    jsonrpc j;

    if (opt1 == "") {
        j["method"] = "private/get_open_orders";
    }
    else if (find(SUPPORTED_CURRENCIES.begin(), SUPPORTED_CURRENCIES.end(), opt1) == SUPPORTED_CURRENCIES.end()) {
        j["method"] = "private/get_open_orders_by_instrument";
        j["params"] = {{"instrument", opt1}};
    }
    else if ( opt2 == "" ) {
        j["method"] = "private/get_open_orders_by_currency";
        j["params"] = {{"currency", opt1}};
    }
    else {
        j["method"] = "private/get_open_orders_by_label";
        j["params"] = {{"currency", opt1},
                        {"label", opt2}};
    }
    return j.dump();
}

string api::modify(const string &input) {
    istringstream is(input);

    int id;
    string cmd;
    string ord_id;
    
    // Correctly parse the input
    is >> id >> cmd >> ord_id;

    if (ord_id.empty()) {
        utils::printerr("Error: Order ID is required\n");
        return "";
    }

    jsonrpc j;
    j["method"] = "private/edit";

    int amount = -1;
    int price = -1;

    // Use utility function to prompt for input
    utils::printcmd("Enter -1 to keep the below parameters the same\n");
    utils::printcmd("Enter the new price of the instrument at which you want to trade (-1 to keep current): ");
    cin >> price;
    utils::printcmd("Enter the new amount you'd like to trade (-1 to keep current): ");
    cin >> amount;

    j["params"] = {{"order_id", ord_id}};
    
    // Only add parameters that are not -1
    if (amount != -1) j["params"]["amount"] = amount;
    if (price != -1) j["params"]["price"] = price;

    return j.dump();
}

string api::cancel(const string &input) {
    istringstream iss(input);
    int id;
    string cmd;
    string ord_id;

    iss >> id >> cmd >> ord_id;
    if (ord_id.empty()) { 
        utils::printerr("Order ID cannot be blank. If you want to cancel all orders, use cancel_all instead.");
        return "";
    }

    jsonrpc j;
    j["method"] = "private/cancel";
    j["params"]["order_id"] = ord_id;
    return j.dump();
}

string api::cancel_all(const string &input) {
    istringstream iss(input);
    int id;
    string cmd;
    string option;
    string label;

    jsonrpc j;
    j["params"] = {};

    iss >> id >> cmd >> option >> label;
    if (option.empty()) { 
        j["method"] = "private/cancel_all";
    }
    else if (find(SUPPORTED_CURRENCIES.begin(), SUPPORTED_CURRENCIES.end(), option) == SUPPORTED_CURRENCIES.end()) {
        j["method"] = "private/cancel_all_by_instrument";
        j["params"]["instrument"] = option;
    }
    else if (option == "-r") {
        j["method"] = "private/cancel_by_label";
        j["params"]["label"] = label;
    }
    else { 
        j["method"] = "private/cancel_all_by_currency";
        j["params"]["currency"] = option;
    }
    
    return j.dump();
}

string api::view_positions(const string &input) {
    istringstream is(input);
    int id;
    string cmd;
    string instrument; // Optional: filter by specific instrument

    is >> id >> cmd >> instrument;

    // Optional: validate instrument format if needed
    if (!instrument.empty() && !is_valid_instrument(instrument)) {
        utils::printerr("Invalid instrument format\n");
        return "";
    }

    jsonrpc j;
    j["method"] = "private/get_positions";
    
    if (!instrument.empty()) {
        j["params"]["instrument_name"] = instrument;
    }

    return j.dump();
}

string api::get_orderbook(const string &input) {
    istringstream is(input);
    int id;
    string cmd;
    string instrument;
    int depth = 10; // Default depth

    is >> id >> cmd >> instrument;
    
    // Validate instrument is not empty
    if (instrument.empty()) {
        utils::printerr("Instrument name is required\n");
        return "";
    }

    // Optional: validate instrument format if needed
    if (!is_valid_instrument(instrument)) {
        utils::printerr("Invalid instrument format\n");
        return "";
    }
    
    // Optional: allow specifying depth
    if (is >> depth) {
        depth = max(1, min(depth, 50)); // Limit depth between 1-50
    }

    jsonrpc j;
    j["method"] = "public/get_orderbook";
    j["params"] = {
        {"instrument_name", instrument},
        {"depth", depth}
    };

    return j.dump();
}

string api::subscribe(const string &input) {


    jsonrpc j;
    j["method"] = "public/subscribe";

    return j.dump();
}