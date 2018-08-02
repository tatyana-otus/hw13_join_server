#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <map>
#include "session.h"

enum class DB_CMD { INSERT, TRUNCATE, INTERSEC, SYMM_DIFF };

struct db_op
{
    using ab_table_t = std::map<size_t, std::string>;

    db_op():valid_reply(false), op_reply(nullptr) {}

    void update()
    {
        valid_reply = false;
    }


    std::shared_ptr<reply_t> get_reply(const ab_table_t& A, const ab_table_t& B)
    {
        if (valid_reply) return op_reply;

        return get_reply_impl(A, B);
    }

    virtual std::shared_ptr<reply_t> get_reply_impl(const ab_table_t& A, const ab_table_t& B) = 0; 

protected:
    bool                        valid_reply;   
    std::shared_ptr<reply_t>    op_reply;
};


struct db_op_intersec: public db_op
{

    std::shared_ptr<reply_t> get_reply_impl(const ab_table_t& A, const ab_table_t& B ) override
    {
        std::unordered_set<size_t> a_id;
        std::set<size_t> intersec;

        a_id.reserve(A.size());
        for(auto const id : A){
            a_id.insert(id.first);
        }

        for(auto const id : B){
            if(a_id.find(id.first) != a_id.end()) {
                intersec.insert(id.first);    
            }
        }

        op_reply = std::make_shared<reply_t>();

        op_reply->reserve(intersec.size());
        for (auto & id : intersec ){
            std::string str;
            str += std::to_string(id) + ",";
            str += A.at(id) + "," + B.at(id) + "\n";
            op_reply->push_back(std::move(str));
        }
        op_reply->push_back("OK\n");

        valid_reply = true;
        return op_reply;
    }
};


struct db_op_symm_diff: public db_op
{

    std::shared_ptr<reply_t> get_reply_impl(const ab_table_t& A, const ab_table_t& B ) override
    {
        std::unordered_set<size_t> a_id;
        std::unordered_set<size_t> b_id;

        std::map<size_t, size_t> symm_diff;

        a_id.reserve(A.size());
        b_id.reserve(B.size());

        for(auto const id : A){
            a_id.insert(id.first);
        }

        for(auto const id : B){
            b_id.insert(id.first);
        }

        for(auto const id : A){
            if(b_id.find(id.first) == b_id.end()) {
                symm_diff[id.first] = 0;    
            }
        }

        for(auto const id : B){
            if(a_id.find(id.first) == a_id.end()) {
                symm_diff[id.first] = 1;    
            }
        }

        op_reply = std::make_shared<reply_t>();

        op_reply->reserve(symm_diff.size());
        for (auto & val : symm_diff ){
            std::string str;
            auto id = val.first;
            str += std::to_string(id) + ",";
            if(val.second == 0)
                str += A.at(id) + "," + "\n";
            else
                str += "," + B.at(id) + "\n";
            
            op_reply->push_back(std::move(str));
        }
        op_reply->push_back("OK\n");

        valid_reply = true;
        return op_reply;
    }
};


struct db_data
{
    db_data(){}

    DB_CMD get_cmd(const std::string & s) const
    {
        return db_command.at(s);
    }


    int on_insert(const cmd_t& cmd)
    {
        if(cmd->size() != 4) throw std::invalid_argument(""); 
        if(!std::all_of(cmd->at(2).begin(), cmd->at(2).end(), ::isdigit))
            throw std::invalid_argument("");             
        auto idx = str_to_idx(cmd->at(1)); // table
        auto id  = str_to_id (cmd->at(2)); // id

        if(tables[idx].find(id) != tables[idx].end()) return 1;
        
        tables[idx][id] = std::move(cmd->at(3));

        op_intersec.update();
        op_symm_diff.update();

        return 0;
    }


    int on_truncate(const cmd_t& cmd)
    {
        if(cmd->size() != 2) throw std::invalid_argument("");
        
        auto idx = str_to_idx(cmd->at(1)); // table

        tables[idx].clear();

        op_intersec.update();
        op_symm_diff.update();

        return 0;
    }  


    auto on_intersection(const cmd_t& cmd)
    {
        if(cmd->size() != 1) throw std::invalid_argument("");

        return op_intersec.get_reply(tables[0], tables[1]);
    }                


    auto on_symmetric_difference(const cmd_t& cmd)
    {
        if(cmd->size() != 1) throw std::invalid_argument("");

        return op_symm_diff.get_reply(tables[0], tables[1]);
    }


private:

    const static int TABLES_NUM = 2;
  
    using table_t    = std::array<std::map<size_t, std::string>, TABLES_NUM>;
    using sd_table_t = std::map<size_t, size_t>;
    using i_table_t  = std::set<size_t>;

    
    size_t str_to_idx(const std::string & s) const
    {
        return tables_idx.at(s);
    }


    size_t str_to_id(const std::string & s) const
    {
        return std::stoull(s);
    }

    db_op_intersec  op_intersec;
    db_op_symm_diff op_symm_diff;

    table_t tables;

    const std::unordered_map<std::string, DB_CMD> db_command{
        { "INSERT",   DB_CMD::INSERT   },{ "INTERSECTION",         DB_CMD::INTERSEC  },     
        { "TRUNCATE", DB_CMD::TRUNCATE },{ "SYMMETRIC_DIFFERENCE", DB_CMD::SYMM_DIFF }
    };

    const std::unordered_map<std::string, size_t> tables_idx{
        { "A", 0 }, { "B", 1 }
    };
};


struct db_worker
{
    db_worker(std::shared_ptr<tasks_t> tasks_):
    quit(false),
    tasks(tasks_){}


    void start(void)
    {
        helper_thread = std::thread( &db_worker::on_cmd,
                                     this);
    } 


    void stop (void) 
    {
        std::unique_lock<std::mutex> lk(tasks->cv_mx);
        quit = true;
        tasks->cv.notify_one();
        lk.unlock();

        helper_thread.join();
    } 

    std::atomic<bool> quit;

private:

    void send_reply_ok(std::weak_ptr<session>& s)
    {
        if (auto spt = s.lock()) { 
            spt->send_reply(std::shared_ptr<reply_t>(&ok_reply, [](reply_t*){}));
        }
    }


    void send_reply_err(std::weak_ptr<session>& s)
    {
        if (auto spt = s.lock()) {
            spt->send_reply(std::shared_ptr<reply_t>(&err_reply, [](reply_t*){}));
        }
    }


    void send_reply_res(std::weak_ptr<session>& s, std::shared_ptr<reply_t> t)
    {
        if (auto spt = s.lock()) {
            spt->send_reply(t);
        }
    }


    void exec_cmd(task_t& t)
    {
        std::weak_ptr<session> s;
        std::shared_ptr<std::vector<std::string>> cmd;
        std::tie(s, cmd) = t;

        try{
            switch(data.get_cmd(cmd->at(0))){
               
            case DB_CMD::INSERT: 
                if(data.on_insert(cmd) == 0){    
                    send_reply_ok(s);
                }
                else{
                    auto str ="ERR duplicate " + cmd->at(2) + "\n"; 
                    send_reply_res(s, std::make_shared<reply_t>(std::initializer_list<std::string>{std::move(str)}));
                }
                break;

            case DB_CMD::TRUNCATE: 
                data.on_truncate(cmd);
                send_reply_ok(s);
                break;

            case DB_CMD::INTERSEC: 
                if(!s.expired()){
                    send_reply_res(s, data.on_intersection(cmd));
                } 
                break;

            case DB_CMD::SYMM_DIFF:
                if(!s.expired()){
                    send_reply_res(s, data.on_symmetric_difference(cmd));
                }
                break;               
            }
        }
        catch(const std::exception &e) {
            send_reply_err(s);
            return;
        }
    }


    void on_cmd() 
    {
        try {
            while(!quit){
                std::unique_lock<std::mutex> lk(tasks->cv_mx);

                tasks->cv.wait(lk, [this](){ return (!this->tasks->empty() || this->quit); });

                if(tasks->empty()) break;

                auto t = tasks->front();
                tasks->pop();
                if (tasks->size() < (MAX_QUEUE_SIZE / 2)) tasks->cv_empty.notify_one();
                lk.unlock();

                exec_cmd(t);                
            } 

            std::unique_lock<std::mutex> lk(tasks->cv_mx);
            while(!tasks->empty()) {
                auto v = tasks->front();
                tasks->pop();
            }
        }
        catch(const std::exception &e) {
            std::lock_guard<std::mutex> lock(tasks->cv_mx);
            tasks->eptr = std::current_exception();          
        }    
    }
   
    std::thread helper_thread;
    std::shared_ptr<tasks_t> tasks; 

    reply_t ok_reply  = {"OK\n"};
    reply_t err_reply = {"ERR wrong command\n"};

    db_data data;
};