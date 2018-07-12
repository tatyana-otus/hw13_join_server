#include <queue>
#include <condition_variable>
#include <atomic>

const size_t MAX_QUEUE_SIZE = 1024;

template<typename T>
struct queue_wrapper
{

    void add(T&& value)
    {
        std::unique_lock<std::mutex> lk_file(cv_mx);
        if(eptr) std::rethrow_exception(eptr);
        
        if(size() > MAX_QUEUE_SIZE) {
            cv.notify_one();
            cv_empty.wait(lk_file, [this](){ 
            return( this->size() <  MAX_QUEUE_SIZE); });
        }

        q_data.push(value);
        cv.notify_one();
        lk_file.unlock();
    }
    

    T& front()
    {
        return q_data.front();
    }


    void pop()
    {

        return q_data.pop();
    }


    bool empty() const
    {
        return q_data.empty();
    }

    
    auto size() const
    {
        return q_data.size();
    }


    std::condition_variable cv;
    std::mutex cv_mx;

    std::condition_variable cv_empty;
    std::mutex cv_mx_empty;

    std::exception_ptr eptr {nullptr};

private:  
    
    void push( T&& value )
    {
        q_data.push(value);
    }


    void push( const T& value )
    {
        q_data.push(value);
    }


    std::queue<T> q_data;
};