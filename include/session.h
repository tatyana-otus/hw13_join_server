#include <boost/asio.hpp>
#include "queue_wrapper.h"
#include <boost/algorithm/string.hpp>

using task_t = std::tuple<std::shared_ptr<session>, std::shared_ptr<std::vector<std::string>>>;
using tasks_t = queue_wrapper<task_t>;


using view_t  = std::vector<std::string>;

class session:
public std::enable_shared_from_this<session>
{
public:

    session(tcp::socket socket_, std::shared_ptr<session_storage> storage_,
            std::shared_ptr<tasks_t> tasks_)
    : socket(std::move(socket_)), ss(storage_),
    tasks(tasks_){}


    void start ()
    {
        ss->add_session(shared_from_this());
        do_read();  
    }


    void do_write(const std::string  msg)
    {
        auto self(shared_from_this());
        boost::asio::async_write(socket,
            boost::asio::buffer(msg,
            msg.length()),
        [this, self](boost::system::error_code ec, std::size_t )
        {
          if (ec)
          {
            //socket.close();
            ss->remove_session(shared_from_this()); 
          }
        });
    }


    void send_reply(std::shared_ptr<view_t> r)
    {
        if(r == nullptr) return;
        reply = r;

        auto self(shared_from_this());
        boost::asio::async_write(socket, reply_to_buffers(),
            [this, self](boost::system::error_code ec, std::size_t )
            {
                reply = nullptr;
                if (ec){
                    //socket.close();
                    ss->remove_session(shared_from_this());    
                }           
            });
    }


    std::vector<boost::asio::const_buffer> reply_to_buffers()
    {
        std::vector<boost::asio::const_buffer> buffers;
     
        for (const auto& s: *reply){
            buffers.push_back(boost::asio::buffer(s));
        }    
        return buffers;
    }

private:
    void do_read()
    {
    auto self(shared_from_this());
    boost::asio::async_read_until(socket,
        buf, '\n',
        [this, self](boost::system::error_code ec, std::size_t )
        {
            if (!ec) {

                std::istream is(&buf);
                std::string cmd;
                std::getline(is, cmd);

                auto cmd_str = std::make_shared<std::vector<std::string>>();
                boost::split(*cmd_str, cmd, boost::is_any_of(" "));

                tasks->add(std::make_tuple(shared_from_this(), cmd_str));
                
                do_read();
            }
            else{
                //socket.close();
                ss->remove_session(shared_from_this());
            }
        });
    }

    tcp::socket socket;
    boost::asio::streambuf buf;
    std::shared_ptr<session_storage> ss;
    std::string cmd;

    std::shared_ptr<tasks_t> tasks;

    std::shared_ptr<view_t> reply;
};