#ifndef __SYLAR_IOMANAGER_H__
#define __SYLAR_IOMANAGER_H__

#include "scheduler.h"
#include "timer.h"

namespace sylar {

// ワークフロー
// 1 register one event -> 2 wait for it to ready -> 3 schedule the callback -> 4 unregister the event -> 5 run the callback
class IOManager : public Scheduler, public TimerManager 
{
public:
    enum Event 
    {
        NONE = 0x0,
        // READ == EPOLLIN
        READ = 0x1,
        // WRITE == EPOLLOUT
        WRITE = 0x4
    };

private:
    struct FdContext 
    {
        struct EventContext 
        {
            // スケジューラ
            Scheduler *scheduler = nullptr;
            // コールバック用コルーチン
            std::shared_ptr<Fiber> fiber;
            // コールバック関数
            std::function<void()> cb;
        };

        // 読み取り event context
        EventContext read; 
        // 書き込み event context
        EventContext write;
        int fd = 0;
        // 登録されたイベント
        Event events = NONE;
        std::mutex mutex;

        EventContext& getEventContext(Event event);
        void resetEventContext(EventContext &ctx);
        void triggerEvent(Event event);        
    };

public:
    IOManager(size_t threads = 1, bool use_caller = true, const std::string &name = "IOManager");
    ~IOManager();

    // add one event at a time
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
    // delete event
    bool delEvent(int fd, Event event);
    // delete the event and trigger its callback
    bool cancelEvent(int fd, Event event);
    // delete all events and trigger its callback
    bool cancelAll(int fd);

    static IOManager* GetThis();

protected:
    void tickle() override;
    
    bool stopping() override;
    
    void idle() override;

    void onTimerInsertedAtFront() override;

    void contextResize(size_t size);

private:
    int m_epfd = 0;
    // ファイルディスクリプタ[0] read，fd[1] write
    int m_tickleFds[2];
    std::atomic<size_t> m_pendingEventCount = {0};
    std::shared_mutex m_mutex;
    // 各ファイルディスクリプタのコンテキストを保存
    std::vector<FdContext *> m_fdContexts;
};

} // end namespace sylar

#endif