#ifndef _THREAD_H_
#define _THREAD_H_

#include <mutex>
#include <condition_variable>
#include <functional>     

namespace sylar
{

// スレッド間の同期に使う
class Semaphore 
{
private:
    std::mutex mtx;                
    std::condition_variable cv;    
    int count;                   

public:
    // セマフォの初期値は0
    explicit Semaphore(int count_ = 0) : count(count_) {}
    
    // P操作
    void wait() 
    {
        std::unique_lock<std::mutex> lock(mtx);
        while (count == 0) { 
            cv.wait(lock); // wait for signals
        }
        count--;
    }

    // V操作
    void signal() 
    {
        std::unique_lock<std::mutex> lock(mtx);
        count++;
        cv.notify_one();  // signal
    }
};

// 2種類のスレッドがある：1. システムが自動で作成するメインスレッド 2. Threadクラスで作成されるスレッド 
class Thread 
{
public:
    Thread(std::function<void()> cb, const std::string& name);
    ~Thread();

    pid_t getId() const { return m_id; }
    const std::string& getName() const { return m_name; }

    void join();

public:
    // システムが割り当てたスレッドIDを取得
	static pid_t GetThreadId();
    // 現在のスレッドを取得
    static Thread* GetThis();

    // 現在のスレッド名を取得
    static const std::string& GetName();
    // 現在のスレッド名を設定
    static void SetName(const std::string& name);

private:
	// スレッド関数
    static void* run(void* arg);

private:
    pid_t m_id = -1;
    pthread_t m_thread = 0;

    // スレッドで実行する関数
    std::function<void()> m_cb;
    std::string m_name;
    
    Semaphore m_semaphore;
};

























}



#endif
