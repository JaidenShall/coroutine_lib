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
    // 時間ヒープからタイマーを削除
    bool cancel();
    // タイマーをリフレッシュ
    bool refresh();
    // タイマーのタイムアウト時間を再設定
    bool reset(uint64_t ms, bool from_now);

private:
    Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager* manager);
 
private:
    // ループするかどうか
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

    // タイマーを追加
    std::shared_ptr<Timer> addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);

    // 条件付きタイマーを追加
    std::shared_ptr<Timer> addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool recurring = false);

    // ヒープ内の最も近いタイムアウト時間を取得
    uint64_t getNextTimer();

    // すべてのタイムアウト済みタイマーのコールバック関数を取得
    void listExpiredCb(std::vector<std::function<void()>>& cbs);

    // ヒープにタイマーがあるかどうか
    bool hasTimer();

protected:
    // 最も早いタイマーがヒープに追加されたとき -> この関数を呼ぶ
    virtual void onTimerInsertedAtFront() {};

    // タイマーを追加
    void addTimer(std::shared_ptr<Timer> timer);

private:
    // システム時間が変化したとき -> この関数を呼ぶ
    bool detectClockRollover();

private:
    std::shared_mutex m_mutex;
    // 時間ヒープ
    std::set<std::shared_ptr<Timer>, Timer::Comparator> m_timers;
    // 次回のgetNextTime()実行前にonTimerInsertedAtFront()が呼び出されたか -> この間に一度だけ呼ばれる
    bool m_tickled = false;
    // 最後にシステム時刻の巻き戻しを確認した絶対時間
    std::chrono::time_point<std::chrono::system_clock> m_previouseTime;
};

}

#endif