#include "websocket/websocket_client.h"
#include "utils/utils.h"
#include "authentication/password.h"
#include <fmt/color.h>


connection_metadata::connection_metadata(int id, websocketpp::connection_hdl hdl, std::string uri):
    m_id(id),
    m_hdl(hdl),
    m_status("Connecting"),
    m_uri(uri),
    m_server("N/A"),
    m_messages({}),
    m_summaries({}),
    MSG_PROCESSED(false)
{}

int connection_metadata::get_id() { return m_id; }
websocketpp::connection_hdl connection_metadata::get_hdl() { return m_hdl; }
std::string connection_metadata::get_status() { return m_status; }

void connection_metadata::record_sent_message(std::string const &message) {
    m_messages.push_back("SENT: " + message);
}

void connection_metadata::record_summary(std::string const &message, std::string const &sent) {
    if (message == "") return;
    json parsed_msg = json::parse(message);
    std::string cmd = parsed_msg.contains("method") ? parsed_msg["method"] : "received";
    std::map<std::string, std::string> summary;
    
    std::map<std::string, std::function<std::map<std::string, std::string>(json)>> action_map = 
    {
        {"public/auth", [](json parsed_msg){ 
                                            std::map<std::string, std::string> summary;
                                            summary["method"] = parsed_msg["method"];
                                            summary["grant_type"] = parsed_msg["params"]["grant_type"];
                                            return summary;
                                         }
        },
        {"private/sell", [](json parsed_msg){
                                            std::map<std::string, std::string> summary = {};
                                            summary["id"] = std::to_string(parsed_msg["id"].get<int>());
                                            summary["method"] = parsed_msg["method"];
                                            summary["instrument_name"] = parsed_msg["params"]["instrument_name"];
                                            if (parsed_msg["params"].contains("amount"))
                                                summary["amount"] = std::to_string(parsed_msg["params"]["amount"].get<int>());
                                            if (parsed_msg["params"].contains("contracts"))
                                                summary["contracts"] = std::to_string(parsed_msg["params"]["contracts"].get<int>());
                                            return summary;
                                         }                   
        },
        {"private/buy", [](json parsed_msg){
                                            std::map<std::string, std::string> summary = {};
                                            summary["id"] = std::to_string(parsed_msg["id"].get<int>());
                                            summary["method"] = parsed_msg["method"];
                                            summary["instrument_name"] = parsed_msg["params"]["instrument_name"];
                                            if (parsed_msg["params"].contains("amount"))
                                                summary["amount"] = std::to_string(parsed_msg["params"]["amount"].get<int>());
                                            if (parsed_msg["params"].contains("contracts"))
                                                summary["contracts"] = std::to_string(parsed_msg["params"]["contracts"].get<int>());
                                            return summary;
                                         }                   
        },
        {"private/cancel", [](json parsed_msg){
                                                std::map<std::string, std::string> summary = {};
                                                summary["method"] = parsed_msg["method"];
                                                summary["id"] = parsed_msg["order_id"];
                                                return summary;
                                              }
        },
        {"received", [](json parsed_msg){
                                        std::map<std::string, std::string> summary = {};
                                        if (parsed_msg.contains("result"))
                                            summary = {{"result", parsed_msg["result"].dump()}};
                                        else if (parsed_msg.contains("error"))
                                            summary = {{"error message", parsed_msg["error"].dump()}};
                                        return summary;
                                     }
        }
    };
    
    auto find = action_map.find(cmd);
    if (find == action_map.end()) {
        summary["id"] = std::to_string(parsed_msg["id"].get<int>());
        if (sent == "SENT") summary["method"] = parsed_msg["method"];
    }
    else {
        summary = find->second(parsed_msg);
    }
    m_summaries.push_back(sent + " : \n" + utils::printmap(summary));
}

void connection_metadata::on_open(client * c, websocketpp::connection_hdl hdl) {
    m_status = "Open";
    client::connection_ptr con = c->get_con_from_hdl(hdl);
    m_server = con->get_response_header("Server");
}

void connection_metadata::on_fail(client * c, websocketpp::connection_hdl hdl) {
    m_status = "Failed";
    client::connection_ptr con = c->get_con_from_hdl(hdl);
    m_server = con->get_response_header("Server");
    m_error_reason = con->get_ec().message();
}

void connection_metadata::on_close(client * c, websocketpp::connection_hdl hdl) {
    m_status = "Closed";
    client::connection_ptr con = c->get_con_from_hdl(hdl);
    std::stringstream s;
    s << "Close code: " << con->get_remote_close_code() << "("
      << websocketpp::close::status::get_string(con->get_remote_close_code())
      << "), Close reason: " << con->get_remote_close_reason();
    
    m_error_reason = s.str();
}

void connection_metadata::on_message(websocketpp::connection_hdl hdl, client::message_ptr msg) {
    if (msg->get_opcode() == websocketpp::frame::opcode::text) {
        m_messages.push_back("RECEIVED: " + msg->get_payload());
        record_summary(msg->get_payload(), "RECEIVED");
    } else {
        m_messages.push_back("RECEIVED: " + websocketpp::utility::to_hex(msg->get_payload()));
        record_summary(websocketpp::utility::to_hex(msg->get_payload()), "RECEIVED");
    }

    char show_msg;
    utils::printcmd("Received message. Show message? Y/N ", 57, 255, 20);
    std::cin >> show_msg;
    if(show_msg == 'y' | show_msg == 'Y'){
        if (msg->get_payload()[0] == '{') {
            std::cout << "Received message: " << utils::pretty(msg->get_payload()) << std::endl;
        }
        else{
            std::cout << "Received message: " << msg->get_payload() << std::endl;
        }
    }

    if (AUTH_SENT) {
        Password::password().setAccessToken(json::parse(msg->get_payload())["result"]["access_token"]);
        utils::printcmd("Authorization successful!\n");
        AUTH_SENT = false;
    }
    MSG_PROCESSED = true;
    cv.notify_one();
}

std::ostream &operator<< (std::ostream &out, connection_metadata const &data) {
    out << "> URI: " << data.m_uri << "\n"
        << "> Status: " << data.m_status << "\n"
        << "> Remote Server: " << (data.m_server.empty() ? "None Specified" : data.m_server) << "\n"
        << "> Error/close reason: " << (data.m_error_reason.empty() ? "N/A" : data.m_error_reason) << "\n"
        << "> Messages Processed: (" << data.m_messages.size() << ") \n";
 
    std::vector<std::string>::const_iterator it;
    for (it = data.m_summaries.begin(); it != data.m_summaries.end(); ++it) {
        out << *it << "\n";
    }
    return out;
}

context_ptr on_tls_init() {
    context_ptr context = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);

    try {
        context->set_options(boost::asio::ssl::context::default_workarounds |
                        boost::asio::ssl::context::no_sslv2 |
                        boost::asio::ssl::context::no_sslv3 |
                        boost::asio::ssl::context::single_dh_use);
    } catch (std::exception &e) {
        std::cout << "Error in context pointer: " << e.what() << std::endl;
    }
    return context;
}

websocket_endpoint::websocket_endpoint(): m_next_id(0) {
    m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
    m_endpoint.clear_error_channels(websocketpp::log::elevel::all);

    m_endpoint.init_asio();
    m_endpoint.start_perpetual();

    m_thread.reset(new websocketpp::lib::thread(&client::run, &m_endpoint));
}

websocket_endpoint::~websocket_endpoint() {
    m_endpoint.stop_perpetual();

    for (con_list::const_iterator it = m_connection_list.begin(); it != m_connection_list.end(); ++it) {
        if (it->second->get_status() != "Open") {
            continue;
        }
        
        std::cout << "> Closing connection " << it->second->get_id() << std::endl;
        
        websocketpp::lib::error_code ec;
        m_endpoint.close(it->second->get_hdl(), websocketpp::close::status::going_away, "", ec);
        if (ec) {
            std::cout << "> Error closing connection " << it->second->get_id() << ": "  
                    << ec.message() << std::endl;
        }
    }
    
    m_thread->join();
}

int websocket_endpoint::connect(std::string const &uri) {
    int new_id = m_next_id++;

    m_endpoint.set_tls_init_handler(websocketpp::lib::bind(
                                    &on_tls_init
                                    ));

    websocketpp::lib::error_code ec;
    client::connection_ptr con = m_endpoint.get_connection(uri, ec);

    if(ec){
        std::cout << "Connection initialization error: " << ec.message() << std::endl;
        return -1;
    }

    connection_metadata::ptr metadata_ptr(new connection_metadata(new_id, con->get_handle(), uri));
    m_connection_list[new_id] = metadata_ptr;

    con->set_open_handler(websocketpp::lib::bind(
                          &connection_metadata::on_open,
                          metadata_ptr,
                          &m_endpoint,
                          websocketpp::lib::placeholders::_1
                          ));

    con->set_fail_handler(websocketpp::lib::bind(
                          &connection_metadata::on_fail,
                          metadata_ptr,
                          &m_endpoint,
                          websocketpp::lib::placeholders::_1
                          ));
    con->set_close_handler(websocketpp::lib::bind(
                           &connection_metadata::on_close,
                           metadata_ptr,
                           &m_endpoint,
                           websocketpp::lib::placeholders::_1
                          ));
    con->set_message_handler(websocketpp::lib::bind(
                             &connection_metadata::on_message,
                             metadata_ptr,
                             websocketpp::lib::placeholders::_1,
                             websocketpp::lib::placeholders::_2
                            ));

    m_endpoint.connect(con);

    return new_id;
}

connection_metadata::ptr websocket_endpoint::get_metadata(int id) const {
    con_list::const_iterator it = m_connection_list.find(id);
    if (it == m_connection_list.end()) {
        return connection_metadata::ptr(); // Return null/empty pointer if not found
    }
    return it->second;
}

void websocket_endpoint::close(int id, websocketpp::close::status::value code, std::string reason) {
    websocketpp::lib::error_code ec;
    
    con_list::iterator it = m_connection_list.find(id);
    if (it == m_connection_list.end()) {
        std::cout << "> No connection found with id " << id << std::endl;
        return;
    }
    
    m_endpoint.close(it->second->get_hdl(), code, reason, ec);
    if (ec) {
        std::cout << "> Error closing connection " << id << ": "  
                  << ec.message() << std::endl;
    }
}

int websocket_endpoint::send(int id, std::string message) {
    websocketpp::lib::error_code ec;
    
    con_list::iterator it = m_connection_list.find(id);
    if (it == m_connection_list.end()) {
        std::cout << "> No connection found with id " << id << std::endl;
        return -1;
    }
    
    m_endpoint.send(it->second->get_hdl(), message, websocketpp::frame::opcode::text, ec);
    
    if (ec) {
        std::cout << "> Error sending message to connection " << id << ": "  
                  << ec.message() << std::endl;
        return -1;
    }
    
    it->second->record_sent_message(message);
    return 0;
}