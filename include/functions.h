#include <iostream>
#include "join_server.h"

using boost::asio::ip::tcp;


void process(const char* port_arg, std::ostream& os = std::cout)
{  
    std::string str_port_arg = port_arg;
    decltype(std::stoull("")) port;

    if(!std::all_of(str_port_arg.begin(), str_port_arg.end(), ::isdigit))
        throw std::invalid_argument("Invalid <port>");

    try {
        port = std::stoull(str_port_arg);
    }    
    catch(const std::exception &e) {
        throw std::invalid_argument("Invalid <port>");
    } 
    
    if(port > 65535){       
        throw std::invalid_argument("Invalid <port>");
    }

    boost::asio::io_service io_service;

    boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);
    signals.async_wait(
    std::bind(&boost::asio::io_service::stop, &io_service));

    tcp::endpoint endpoint(tcp::v4(), port);

    join_server server(io_service, endpoint, os);
  
    io_service.run();
}