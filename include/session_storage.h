#include <iostream>
#include <boost/asio.hpp>
#include <set>
//#include "command.h"
//#include "bulk_handlers.h"

using boost::asio::ip::tcp;

class session;

class session_storage
{   
public:

    session_storage()
    {}


    public:
    void add_session(std::shared_ptr<session> s) 
    {
        sessions.insert(s);
    };

    void remove_session(std::shared_ptr<session> s)
    {
        sessions.erase(s);
    };

private:

    std::set<std::shared_ptr<session>> sessions;
};