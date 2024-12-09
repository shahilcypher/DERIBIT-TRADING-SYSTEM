#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <functional>

#include <websocketpp/config/asio_client.hpp> 
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ssl/context.hpp> 
#include <websocketpp/client.hpp> 

#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

extern bool AUTH_SENT;

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef std::shared_ptr<boost::asio::ssl::context> context_ptr;

class connection_metadata {
private:
    int m_id;
    websocketpp::connection_hdl m_hdl;
    std::string m_status;
    std::string m_uri;
    std::string m_server;
    std::string m_error_reason;
    std::vector<std::string> m_summaries;

public:
    typedef websocketpp::lib::shared_ptr<connection_metadata> ptr;

    std::mutex mtx;
    std::condition_variable cv;
    std::vector<std::string> m_messages;
    bool MSG_PROCESSED;

    connection_metadata(int id, websocketpp::connection_hdl hdl, std::string uri);

    int get_id();
    websocketpp::connection_hdl get_hdl();
    std::string get_status();
    void record_sent_message(std::string const &message);
    void record_summary(std::string const &message, std::string const &sent);

    void on_open(client * c, websocketpp::connection_hdl hdl);
    void on_fail(client * c, websocketpp::connection_hdl hdl);
    void on_close(client * c, websocketpp::connection_hdl hdl);
    void on_message(websocketpp::connection_hdl hdl, client::message_ptr msg);

    friend std::ostream &operator<< (std::ostream &out, connection_metadata const &data);
};

context_ptr on_tls_init();

class websocket_endpoint {
private:
    typedef std::map<int, connection_metadata::ptr> con_list;

    client m_endpoint;
    websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_thread;

    con_list m_connection_list;
    int m_next_id;

public:
    websocket_endpoint();
    ~websocket_endpoint();

    int connect(std::string const &uri);
    connection_metadata::ptr get_metadata(int id) const;
    void close(int id, websocketpp::close::status::value code, std::string reason);
    int send(int id, std::string message);
};

#endif // WEBSOCKET_CLIENT_H