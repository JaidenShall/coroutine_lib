#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "hook.h"
#include "fiber.h"
#include "thread.h"

#include <mutex>
#include <vector>

namespace sylar {

class Scheduler
{
public:
	Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name="Scheduler");
	virtual ~Scheduler();
	
	const std::string& getName() const {return m_name;}

public:	
	// 実行中のスケジューラを取得
	static Scheduler* GetThis();

protected:
	// 実行中のスケジューラを設定
	void SetThis();
	
public:	
	// タスクをタスクリストに追加
    template <class FiberOrCb>
    void scheduleLock(FiberOrCb fc, int thread = -1) 
    {
    	bool need_tickle;
    	{
    		std::lock_guard<std::mutex> lock(m_mutex);
    		// empty ->  all thread is idle -> need to be waken up
    		need_tickle = m_tasks.empty();
	        
	        ScheduleTask task(fc, thread);
	        if (task.fiber || task.cb) 
	        {
	            m_tasks.push_back(task);
	        }
    	}
    	
    	if(need_tickle)
    	{
    		tickle();
    	}
    }
	
	// 启动线程池
	virtual void start();
	// 关闭线程池
	virtual void stop();	
	
protected:
	virtual void tickle();
	
	// スレッド関数
	virtual void run();

	// アイドルコルーチン関数
	virtual void idle();
	
	// 停止可能かどうか
	virtual bool stopping();

	bool hasIdleThreads() {return m_idleThreadCount>0;}

private:
	// タスク
	struct ScheduleTask
	{
		std::shared_ptr<Fiber> fiber;
		std::function<void()> cb;
		int thread; // タスクを実行すべきスレッドID

		ScheduleTask()
		{
			fiber = nullptr;
			cb = nullptr;
			thread = -1;
		}

		ScheduleTask(std::shared_ptr<Fiber> f, int thr)
		{
			fiber = f;
			thread = thr;
		}

		ScheduleTask(std::shared_ptr<Fiber>* f, int thr)
		{
			fiber.swap(*f);
			thread = thr;
		}	

		ScheduleTask(std::function<void()> f, int thr)
		{
			cb = f;
			thread = thr;
		}		

		ScheduleTask(std::function<void()>* f, int thr)
		{
			cb.swap(*f);
			thread = thr;
		}

		void reset()
		{
			fiber = nullptr;
			cb = nullptr;
			thread = -1;
		}	
	};

private:
	std::string m_name;
	// ミューテックス -> タスクキューを保護
	std::mutex m_mutex;
	// スレッドプール
	std::vector<std::shared_ptr<Thread>> m_threads;
	// タスク队列
	std::vector<ScheduleTask> m_tasks;
	// ワーカースレッドのIDを格納
	std::vector<int> m_threadIds;
	// 追加作成が必要なスレッド数
	size_t m_threadCount = 0;
	// アクティブスレッド数
	std::atomic<size_t> m_activeThreadCount = {0};
	// アイドルスレッド数
	std::atomic<size_t> m_idleThreadCount = {0};

	// メインスレッドをワーカースレッドとして使うか
	bool m_useCaller;
	// その場合 -> スケジューラコルーチンを追加作成
	std::shared_ptr<Fiber> m_schedulerFiber;
	// その場合 -> メインスレッドのIDを記録
	int m_rootThread = -1;
	// 現在停止中かどうか
	bool m_stopping = false;	
};

}

#endif