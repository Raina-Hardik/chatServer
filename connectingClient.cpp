#include <deque>
#include <array>
#include <thread>
#include <iostream>
#include <cstring>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <chrono>
#include "protocol.hpp"

// #define ASIO_STANDALONE							// Still demands boost support?? Not going to bother figuring it out

using boost::asio::ip::tcp;							// Standalone ASIO doesn't have TCP for some reason DESPITE being nested

class client {
public:
	// Constructor
    client(const std::array<char, MAX_NICKNAME>& username, boost::asio::ioServiceHandler& ioServiceHandler, tcp::resolver::iterator tcpEndpoint) :
            asioService(ioServiceHandler), socketCon(ioServiceHandler) {
            
        strcpy(nickName.data(), username.data());
        memset(msgReader.data(), '\0', MAX_IP_PACK_SIZE);			// Read ASCAIZ (Null Terminated String)
        boost::asio::async_connect(socketCon, tcpEndpoint, boost::bind(&client::onConnect, this, _1));
        
        // Starts the async connection on buffer, works like Producer - Consumer problem. 
        // On the Socket, to the tcpEndpoint, bind 'this' client to the server.
    }

    void write(const std::array<char, MAX_IP_PACK_SIZE>& msg) {			// Call Write Handler
        asioService.post(boost::bind(&client::writeImplement, this, msg));
    }

    void close() {
        asioService.post(boost::bind(&client::closeImplement, this));		// Call Close Handler
    }

private:

    void onConnect(const boost::system::error_code& error) {
        if (!error) {								// On success, write to Nickname Buffer on Socket
            boost::asio::async_write(socketCon,
                                     boost::asio::buffer(nickName, nickName.size()),
                                     boost::bind(&client::readHandler, this, _1));
        }
    }

    void readHandler(const boost::system::error_code& error) {
        std::cout << msgReader.data() << std::endl;
        if (!error) {								// On success, read from Nickname Buffer on Socket
            boost::asio::async_read(socketCon,
                                    boost::asio::buffer(msgReader, msgReader.size()),
                                    boost::bind(&client::readHandler, this, _1));
        } else {								
            closeImplement();
        }
    }
    
    void writeHandler(const boost::system::error_code& error) {
        if (!error) {
            msgWriter.pop_front();			// Complete Unit Work
            if (!msgWriter.empty()) {			// Write remaining data
                boost::asio::async_write(socketCon,
                                         boost::asio::buffer(msgWriter.front(), msgWriter.front().size()),
                                         boost::bind(&client::writeHandler, this, _1));
            }
        } else {
            closeImplement();				// Data Written, close writer
    }

    void writeImplement(std::array<char, MAX_IP_PACK_SIZE> msg) {
        msgWriter.push_back(msg);			// Add to Writing Queue
        if (msgWriter.empty()) {			// Writing in progress
            boost::asio::async_write(socketCon,
                                     boost::asio::buffer(msgWriter.front(), msgWriter.front().size()),
                                     boost::bind(&client::writeHandler, this, _1));
        }
    }

    void closeImplement() {
        socketCon.close();				// Close Handler and Connection
    }

    boost::asio::ioServiceHandler& asioService;		// Asynch IO Service Client
    tcp::socket socketCon;				// TCP Socket for Connection
    std::array<char, MAX_IP_PACK_SIZE> msgReader;	// Size of Message Defined in protocol against buffer overflow for reader-writer
    std::deque<std::array<char, MAX_IP_PACK_SIZE>> msgWriter;		
    std::array<char, MAX_NICKNAME> nickName;		// Small array/string for name
};

int main(int argc, char* argv[]){
    try {
        if (argc != 4) {
            std::cerr << "Usage: client <username> <host> <port>\n";
            return 1;
        }
        
        // Must connect to host with username over a port
        // Each port identifies different "chatroom", each isolated
        // The isolation ensures isolation of transfer data to each "room"
        
        
        boost::asio::ioServiceHandler ioServiceHandler;			// IO service handler
        tcp::resolver resolver(ioServiceHandler);			// Scope and reference resolver
        tcp::resolver::query query(argv[2], argv[3]);			// TCP host connection on host:port
        tcp::resolver::iterator iterator = resolver.resolve(query);	// Resolve connection
        std::array<char, MAX_NICKNAME> username;			// Store nickname
        strcpy(username.data(), argv[1]);

        client cli(username, ioServiceHandler, iterator);		// Start and link client
									// Start concurrent thread for client
        std::thread t(boost::bind(&boost::asio::ioServiceHandler::run, &ioServiceHandler));

        std::array<char, MAX_IP_PACK_SIZE> msg;

        while(1) {
            memset(msg.data(), '\0', msg.size());			// Set memory buffer to NULL 
            if (!std::cin.getline(msg.data(), MAX_IP_PACK_SIZE - PADDING - MAX_NICKNAME)) {
                std::cin.clear();					// Fetch memory after padding and clear system IO buffer
            }
            cli.write(msg);						// Write fetched message
        }

        cli.close();							// Close client connection
        t.join();							// And join thread to main stream
    } catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << "\n";			// DEBUG
    }

    return 0;
}
