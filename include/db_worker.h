#include <fstream>
#include <thread>
#include <unordered_map>
#include <queue>
#include <set>
#include <map>

enum class DB_CMD { INSERT, TRUNCATE, INTERSEC, SYMM_DIFF };

struct db_data
{
    db_data():
    view_intersec_valid(false),
    view_sym_diff_valid(false){}


    DB_CMD get_cmd(const std::string & s) const
    {
        return db_command.at(s);
    }


    int on_insert(const std::shared_ptr<std::vector<std::string>>& cmd)
    {
        if(cmd->size() != 4) throw std::invalid_argument(""); 
        if(!std::all_of(cmd->at(2).begin(), cmd->at(2).end(), ::isdigit))
            throw std::invalid_argument("");             
        auto idx = str_to_idx(cmd->at(1)); // table
        auto id  = str_to_id (cmd->at(2)); // id

        if(insert(idx, id, cmd->at(3)) == 0){
            view_intersec_valid = false;
            view_sym_diff_valid = false;
            return 0;
        }  
        return 1; 
    }


    int on_truncate(const std::shared_ptr<std::vector<std::string>>& cmd)
    {
        if(cmd->size() != 2) throw std::invalid_argument("");
        
        auto idx = str_to_idx(cmd->at(1)); // table

        truncate(idx);
        view_intersec_valid = false;
        view_sym_diff_valid = false;

        return 0;
    }  


    auto on_intersection(const std::shared_ptr<std::vector<std::string>>& cmd)
    {
        if(cmd->size() != 1) throw std::invalid_argument("");

        if(!view_intersec_valid)
            validate_intersec_view(); 

        return view_intersec;
    }                


    auto on_symmetric_difference(const std::shared_ptr<std::vector<std::string>>& cmd)
    {
        if(cmd->size() != 1) throw std::invalid_argument("");

        if(!view_sym_diff_valid)
            validate_symm_diff();

        return view_sym_diff;
    }


private:

    const static int TABLES_NUM = 2;
  
    using table_t    = std::array<std::map<size_t, std::string>, TABLES_NUM>;
    using sd_table_t = std::map<size_t, size_t>;
    using i_table_t  = std::set<size_t>;


    int insert( size_t idx, size_t id, const std::string & s)
    {
        assert(idx < TABLES_NUM);

        if(tables[idx].find(id) != tables[idx].end()) return -1;
        
        tables[idx][id] = s;

        auto it = symm_diff.find(id);
        if( it == symm_diff.end() ){
            symm_diff[id] = idx;
        }
        else{
            intersec.insert(id);
            symm_diff.erase(it);                             
        }

        return 0;
    }


    void truncate(size_t idx)
    {
        assert(idx < TABLES_NUM);

        tables[idx].clear();

        for (auto it = symm_diff.cbegin(); it != symm_diff.cend(); ){
            if(it->second == idx) 
                it = symm_diff.erase(it);
            else ++it;
        }

        for (auto & id : intersec ){
            if(idx == 0)
                symm_diff[id] = 1;
            else
                symm_diff[id] = 0;
        }   

        intersec.clear();
    }


    void validate_intersec_view()
    {
        view_intersec = std::make_shared<reply_t>();

        view_intersec->reserve(intersec.size());
        for (auto & id : intersec ){
            std::string str;
            str += std::to_string(id) + ",";
            str += tables[0][id] + "," + tables[1][id] + "\n";
            view_intersec->push_back(std::move(str));
        }
        view_intersec->push_back("OK\n");
    }


    void validate_symm_diff()
    {
        view_sym_diff = std::make_shared<reply_t>();

        view_sym_diff->reserve(symm_diff.size());
        for (auto & val : symm_diff ){
            std::string str;
            auto id = val.first;
            str += std::to_string(id) + ",";
            if(val.second == 0)
                str += tables[0][id] + "," + "\n";
            else
                str += "," + tables[1][id] + "\n";
            
            view_sym_diff->push_back(std::move(str));
        }
        view_sym_diff->push_back("OK\n");
    }

    
    size_t str_to_idx(const std::string & s)
    {
        return tables_idx.at(s);
    }


    size_t str_to_id(const std::string & s)
    {
        return std::stoull(s);
    }

    table_t tables;

    i_table_t  intersec;
    sd_table_t symm_diff;

    std::shared_ptr<reply_t> view_intersec;
    std::shared_ptr<reply_t> view_sym_diff;

    bool view_intersec_valid;
    bool view_sym_diff_valid;

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