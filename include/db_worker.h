#include <fstream>
#include <thread>
#include <unordered_map>
#include <queue>

struct db_worker
{
    db_worker(std::shared_ptr<tasks_t> tasks_):
    quit(false),
    tasks(tasks_),   
    view_intersec_valid(false),
    view_sym_diff_valid(false) {}


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
    using table_t = std::map<size_t, std::array<std::string, 2>>;

    auto get_idx(const std::string & s)
    {
        return tables_idx.at(s);
    }


    auto get_id(const std::string & s)
    {
        return std::stoull(s);
    }


    int insert( size_t idx, size_t id, const std::string & s)
    {
        if(intersection.find(id) != intersection.end()) return -1;

        auto it = symmetric_difference.find(id);
        if( it == symmetric_difference.end() ){
            if(idx == 0)
                symmetric_difference[id] = { {std::move(s), ""} };
            else
                symmetric_difference[id] = { {"", std::move(s)} };
        }
        else{
            if(it->second[idx] == ""){
                intersection[id] = std::move(it->second); 
                intersection[id][idx] = std::move(s);  

                symmetric_difference.erase(it); 
            }               
            else return -1;                
        }
        return 0;
    }


    void truncate(size_t idx)
    {
        for (auto it = symmetric_difference.cbegin(); it != symmetric_difference.cend(); ){
            if (it->second[idx] !=  "") 
                it = symmetric_difference.erase(it); 
            else ++it;
        }

        for (auto & val : intersection ){
            val.second[idx] = "";
            symmetric_difference.insert(std::move(val));
        }
        intersection.clear();
    }


    void post_reply(std::shared_ptr<session>& s, std::shared_ptr<view_t>  v)
    {
        s->send_reply(v);
    }


    void validate_table_view(table_t & t, std::shared_ptr<view_t> & v)
    {
        v = std::make_shared<view_t>();

        v->reserve(t.size());
        for (auto & val : t ){
            std::string str;
            str += std::to_string(val.first) + ",";
            str += val.second[0] + "," + val.second[1] + "\n";
            v->push_back(std::move(str));
        }
        v->push_back("OK\n");
    }

    
    void exec_cmd(task_t& t)
    {
        std::shared_ptr<session> s;
        std::shared_ptr<std::vector<std::string>> cmd;
        std::tie(s, cmd) = t;

        try{
            if(cmd->at(0) == "INSERT"){ //INSERT table id name 
                if(cmd->size() != 4) throw std::invalid_argument(""); 
                if(!std::all_of(cmd->at(2).begin(), cmd->at(2).end(), ::isdigit))
                    throw std::invalid_argument("Invalid <port>");             
                auto idx = get_idx(cmd->at(1)); // table
                auto id  = get_id (cmd->at(2)); // id
                if(insert(idx, id, cmd->at(3)) == 0){
                    view_intersec_valid = false;
                    view_sym_diff_valid = false;
                    s->do_write(ok_reply);
                }
                else s->do_write("ERR duplicate " + cmd->at(2) + "\n");                                   
            }
            
            else if(cmd->at(0) == "TRUNCATE"){ // TRUNCATE table
                if(cmd->size() != 2) throw std::invalid_argument("");
                auto idx = get_idx(cmd->at(1)); // table
                truncate(idx);
                view_intersec_valid = false;
                view_sym_diff_valid = false;
                s->do_write(ok_reply);
            }

            else if(cmd->at(0) == "INTERSECTION"){
                if(cmd->size() != 1) throw std::invalid_argument("");
                if(!view_intersec_valid)
                    validate_table_view(intersection, view_intersec);
                post_reply(s, view_intersec);
            }

            else if(cmd->at(0) == "SYMMETRIC_DIFFERENCE"){
                if(cmd->size() != 1) throw std::invalid_argument("");
                if(!view_sym_diff_valid)
                    validate_table_view(symmetric_difference, view_sym_diff);
                post_reply(s, view_sym_diff);
            }

            else throw std::invalid_argument("");
        }
        catch(const std::exception &e) {
            s->do_write(er_reply);
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

    const std::unordered_map<std::string, size_t> tables_idx{
        { "A", 0 }, { "B", 1 }
    };


    const std::string ok_reply = "OK\n"; 
    const std::string er_reply = "ERR wrong command\n";

    table_t intersection; 
    table_t symmetric_difference;

    std::shared_ptr<view_t> view_intersec;
    std::shared_ptr<view_t> view_sym_diff;

    bool view_intersec_valid;
    bool view_sym_diff_valid;
};