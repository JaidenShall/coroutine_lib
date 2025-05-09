#ifndef __SYLAR_TIMER_H__
#define __SYLAR_TIMER_H__

#include <memory>
#include <vector>
#include <set>
#include <shared_mutex>
#include <assert.h>
#include <functional>
#include <mutex>

namespace sylar {

class TimerManager;

class Timer : public std::enable_shared_from_this<Timer> 
{
    friend class TimerManager;
public:
    // 時間ヒープからタイマーを削除する
    bool cancel();
    // タイマーを更新する
    bool refresh();
    // タイマーのタイムアウト時間をリセットする
    bool reset(uint64_t ms, bool from_now);

private:
    Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager* manager);
 
private:
    // 繰り返し実行かどうか
    bool m_recurring = false;
    // タイムアウト時間
    uint64_t m_ms = 0;
    // 絶対タイムアウト時間
    std::chrono::time_point<std::chrono::system_clock> m_next;
    // タイムアウト時に実行されるコールバック関数
    std::function<void()> m_cb;
    // このタイマーを管理するマネージャ
    TimerManager* m_manager = nullptr;

private:
    // 最小ヒープ用の比較関数
    struct Comparator 
    {
        bool operator()(const std::shared_ptr<Timer>& lhs, const std::shared_ptr<Timer>& rhs) const;
    };
};

class TimerManager 
{
    friend class Timer;
public:
    TimerManager();
    virtual ~TimerManager();

    // タイマーを追加する
    std::shared_ptr<Timer> addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);

    // 条件付きタイマーを追加する
    std::shared_ptr<Timer> addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool recurring = false);

    // ヒープ中で最も近いタイムアウト時間を取得する
    uint64_t getNextTimer();

    // すべてのタイムアウトタイマーのコールバック関数を取得する
    void listExpiredCb(std::vector<std::function<void()>>& cbs);

    // ヒープにタイマーが存在するかどうか
    bool hasTimer();

protected:
    // 最も早いタイマーがヒープに追加されたときに呼ばれる関数
    virtual void onTimerInsertedAtFront() {};

    // タイマーを追加する
    void addTimer(std::shared_ptr<Timer> timer);

private:
    // システム時間が変更されたときに呼ばれる関数
    bool detectClockRollover();

private:
    std::shared_mutex m_mutex;
    // 时间堆
    std::set<std::shared_ptr<Timer>, Timer::Comparator> m_timers;
    // 在下次getNextTime()执行前 onTimerInsertedAtFront()是否已经被触发了 -> 在此过程中 onTimerInsertedAtFront()只执行一次
    bool m_tickled = false;
    // 上次检查系统时间是否回退的绝对时间
    std::chrono::time_point<std::chrono::system_clock> m_previouseTime;
};

}

#endif