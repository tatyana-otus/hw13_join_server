#include <boost/asio.hpp>
#include "db_worker.h"


class join_server
{
public:
    join_server(boost::asio::io_service& io_service,
      const tcp::endpoint& endpoint, std::ostream& os)
    : acceptor(io_service, endpoint),
      socket(io_service)
    {
        ss = std::make_shared<session_storage>();

        q_db_tasks = std::make_shared<tasks_t>();

        db = std::make_shared<db_worker>(q_db_tasks);
        db->start();
        do_accept();
    }

    ~join_server()
    {
        try {  
            db->stop();
        }
        catch(const std::exception &e) {

        }
    }

private:
    void do_accept()
    {
        acceptor.async_accept(socket,
            [this](boost::system::error_code ec)
            {
              if (!ec)
            {
                std::make_shared<session>(std::move(socket), ss, q_db_tasks)->start();
            }

            do_accept();
            
            });
    }

    tcp::acceptor acceptor;
    tcp::socket socket;
    std::shared_ptr<session_storage> ss;
    std::shared_ptr<tasks_t> q_db_tasks;

    std::shared_ptr<db_worker> db;
};