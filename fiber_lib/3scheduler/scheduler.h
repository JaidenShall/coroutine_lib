#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

//#include "hook.h"
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
	// 現在実行中のスケジューラを取得
	static Scheduler* GetThis();

protected:
	// 現在実行中のスケジューラを設定
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

	// 空闲协程函数
	virtual void idle();
	
	// 是否可以关闭
	virtual bool stopping();

	bool hasIdleThreads() {return m_idleThreadCount>0;}

private:
	// 任务
	struct ScheduleTask
	{
		std::shared_ptr<Fiber> fiber;
		std::function<void()> cb;
		int thread; // 指定任务需要运行的线程id

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
	// ミューテックス -> タスクリストを保護
	std::mutex m_mutex;
	// スレッドプール
	std::vector<std::shared_ptr<Thread>> m_threads;
	// タスクリスト
	std::vector<ScheduleTask> m_tasks;
	// 存储工作线程的线程id
	std::vector<int> m_threadIds;
	// 需要额外创建的线程数
	size_t m_threadCount = 0;
	// 活跃线程数
	std::atomic<size_t> m_activeThreadCount = {0};
	// 空闲线程数
	std::atomic<size_t> m_idleThreadCount = {0};

	// メインスレッドがワーカースレッドとして使われるか
	bool m_useCaller;
	// そうであれば -> スケジューラコルーチンを作成する必要がある
	std::shared_ptr<Fiber> m_schedulerFiber;
	// そうであれば -> メインスレッドのIDを記録する
	int m_rootThread = -1;
	// 現在シャットダウン中かどうか
	bool m_stopping = false;	
};

}

#endif