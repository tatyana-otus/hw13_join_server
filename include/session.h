#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include "queue_wrapper.h"

class session;

using boost::asio::ip::tcp;

using task_t = std::tuple<std::weak_ptr<session>, std::shared_ptr<std::vector<std::string>>>;
using tasks_t = queue_wrapper<task_t>;

using reply_t  = std::vector<std::string>;

class session:
public std::enable_shared_from_this<session>
{
public:

    session(tcp::socket socket_,
            std::shared_ptr<tasks_t> tasks_)
    : socket(std::move(socket_)),
    tasks(tasks_), cur_reply(nullptr){}


    void start ()
    {
        do_read();  
    }


    void send_reply(std::shared_ptr<reply_t> r)
    {
        std::lock_guard<std::mutex> lock(mx_reply); 
        q_reply.push(r);
        if(cur_reply == nullptr){
            cur_reply = q_reply.front();
            q_reply.pop();
            do_write();
        }
    }

private:

    void do_write()
    { 
        auto self(shared_from_this());
        boost::asio::async_write(socket, reply_to_buffers(),
            [this, self](boost::system::error_code ec, std::size_t )
            {
                
                if (!ec){
                    std::lock_guard<std::mutex> lock(mx_reply);
                    cur_reply = nullptr;
                    if(!q_reply.empty()){
                        cur_reply = q_reply.front();
                        q_reply.pop();
                        do_write();    
                    }    
                }          
            });
    }

    
    std::vector<boost::asio::const_buffer> reply_to_buffers()
    {
        std::vector<boost::asio::const_buffer> buffers;
     
        for (const auto& s: *cur_reply){
            buffers.push_back(boost::asio::buffer(s));
        }    
        return buffers;
    }

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
        });
    }

    tcp::socket socket;
    boost::asio::streambuf buf;
    std::string cmd;

    std::shared_ptr<tasks_t> tasks;

    std::shared_ptr<reply_t> cur_reply;
    std::queue<std::shared_ptr<reply_t>>q_reply; 
    std::mutex mx_reply;
};