#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <fmt/color.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "websocket/websocket_client.h"
#include "api/api.h"
#include "utils/utils.h"
#include "authentication/password.h"

int main() {
    // Flag to control the main loop
    bool done = false;
    char* input;
    
    // Create a websocket endpoint instance
    websocket_endpoint endpoint;

    // Print a colorful welcome banner
    fmt::print(fg(fmt::color::crimson) | fmt::emphasis::bold, 
    "-------------------------- DERIBIT Backend --------------------------\n");
    std::cout << "Type 'help' to check out all available commands\n" << std::endl;
              
    // Main command processing loop
    while (!done) {
        // Readline provides a prompt and stores the history
        input = readline(fmt::format(fg(fmt::color::green), "deribit> ").c_str());
        if (!input) {
            break; // Exit on EOF (Ctrl+D)
        }

        std::string command(input);
        free(input); // Clean up the memory allocated by readline

        // Skip empty input but store valid commands in history
        if (command.empty()) {
            continue;
        }
        add_history(command.c_str());

        // Command processing
        if (command == "quit" || command == "exit") {
            done = true;
        } 
        else if (command == "help") {
            // Help command - print out all available commands
            std::cout << "\nCOMMAND LIST:\n"
            << "> help: Displays this help text\n"
            << "> quit / exit: exits the program\n"
            << "> connect <URI>: creates a connection with the given URI\n"
            << "> close <id> <code (optional)> <reason (optional)>: closes the connection with the given id with optionally specifiable exit_code and/or reason\n"
            << "> show <id>: Gets the metadata of the connection with the given id\n"
            << "> show_messages <id>: Gets the complete messages sent and received on the connection with the given id\n"
            << "> send <id> <message>: Sends the message to the specified connection\n\n"
            << "Deribit API COMMANDS\n"
            << "> Deribit connect: Creates a new connection to the Deribit testnet website\n"
            << "> Deribit <id> authorize <client_id> <client_secret>: sends the authorization message to retrieve the access token\n\tAn optional flag -r can be set to remember the access_token for the rest of the session\n"
            << "> Deribit <id> buy <instrument> <comments>: Sends a buy order via the connection with id <id> for the instrument specified\n"
            << "> Deribit <id> sell <instrument> <comments>: Sends a sell order via the connection with id <id> for the instrument specified\n"
            << "> Deribit <id> get_open_orders {options}: Gets all the open orders\n"
            << "> Deribit <id> modify <order_id>: Allows you to modify the price and amount of an active order with known order id\n"
            << "> Deribit <id> cancel <order_id>: Allows you to cancel a specific order\n"
            << std::endl;
        }
        else if (command.substr(0, 7) == "connect") {
        // Check if URI is provided
        if (command.length() <= 8) {
            fmt::print(fg(fmt::color::red), "Error: Missing URI. Usage: connect <URI>\n");
        } else {
            std::string uri = command.substr(8);
            int id = endpoint.connect(uri);
            if (id != -1) {
                fmt::print(fg(fmt::color::green), "> Created connection with id {}\n", id);
                std::cout << "> Status: " << endpoint.get_metadata(id)->get_status() << std::endl;
            }else {
                 fmt::print(fg(fmt::color::red), "Error: Failed to create connection.\n");
                }
            }
        }
        else if (command.substr(0, 13) == "show_messages") {
            // Show messages for a specific connection
            int id = atoi(command.substr(14).c_str());
 
            connection_metadata::ptr metadata = endpoint.get_metadata(id);
            for (const auto& msg : metadata->m_messages) {
                std::cout << msg << "\n\n";
            }
        }
        else if (command.substr(0, 4) == "show") {
            // Show metadata for a specific connection
            int id = atoi(command.substr(5).c_str());
 
            connection_metadata::ptr metadata = endpoint.get_metadata(id);
            if (metadata) {
                std::cout << *metadata << std::endl;
            } else {
                std::cout << "> Unknown connection id " << id << std::endl;
            }
        }
        else if (command.substr(0, 5) == "close") {
            // Close a specific connection
            std::stringstream ss(command);

            std::string cmd;
            int id;
            int close_code = websocketpp::close::status::normal;
            std::string reason;

            ss >> cmd >> id >> close_code;
            std::getline(ss, reason);

            endpoint.close(id, close_code, reason);
        }
        else if (command.substr(0, 4) == "send") {
            // Send a message on a specific connection
            std::stringstream ss(command);
                
            std::string cmd;
            int id;
            std::string message = "";
            
            ss >> cmd >> id;
            std::getline(ss, message);
            
            endpoint.send(id, message);

            // Wait for message processing
            std::unique_lock<std::mutex> lock(endpoint.get_metadata(id)->mtx);
            endpoint.get_metadata(id)->cv.wait(lock, [&] { return endpoint.get_metadata(id)->MSG_PROCESSED; });
            endpoint.get_metadata(id)->MSG_PROCESSED = false;
        }
        else if (command == "Deribit connect") {
            // Special Deribit connection
            std::string uri = "wss://test.deribit.com/ws/api/v2";
            int id = endpoint.connect(uri);
            if (id != -1) {
                std::cout << "> Created connection to Deribit TESTNET with id " << id << std::endl;
                std::cout << "> Status: " << endpoint.get_metadata(id)->get_status() << std::endl;
            }
        }
        else if (command.substr(0, 7) == "Deribit") {
            // Process Deribit-specific API commands
            int id; 
            std::string cmd;

            std::stringstream ss(command);
            ss >> cmd >> id;
            
            std::string msg = api::process(command);
            if (msg != "") {
                int success = endpoint.send(id, msg);
                if (success >= 0) {
                    std::unique_lock<std::mutex> lock(endpoint.get_metadata(id)->mtx);
                    endpoint.get_metadata(id)->cv.wait(lock, [&] { return endpoint.get_metadata(id)->MSG_PROCESSED; });
                    endpoint.get_metadata(id)->MSG_PROCESSED = false;
                }
            }
        }
        else {
            std::cout << "Unrecognized command" << std::endl;
        }
    }
    return 0;
}
