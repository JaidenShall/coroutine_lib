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
	// (JP) 获取正在运行的调度器
	static Scheduler* GetThis();

protected:
	// (JP) 设置正在运行的调度器
	void SetThis();
	
public:	
	// (JP) 添加任务到任务队列
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
	
	// (JP) 启动线程池
	virtual void start();
	// (JP) 关闭线程池
	virtual void stop();	
	
protected:
	virtual void tickle();
	
	// (JP) 线程函数
	virtual void run();

	// (JP) 空闲协程函数
	virtual void idle();
	
	// (JP) 是否可以关闭
	virtual bool stopping();

	bool hasIdleThreads() {return m_idleThreadCount>0;}

private:
	// (JP) 任务
	struct ScheduleTask
	{
		std::shared_ptr<Fiber> fiber;
		std::function<void()> cb;
		in(JP) t thread; // 指定任务需要运行的线程id

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
	// (JP) 互斥锁 -> 保护任务队列
	std::mutex m_mutex;
	// (JP) 线程池
	std::vector<std::shared_ptr<Thread>> m_threads;
	// (JP) 任务队列
	std::vector<ScheduleTask> m_tasks;
	// (JP) 存储工作线程的线程id
	std::vector<int> m_threadIds;
	// (JP) 需要额外创建的线程数
	size_t m_threadCount = 0;
	// (JP) 活跃线程数
	std::atomic<size_t> m_activeThreadCount = {0};
	// (JP) 空闲线程数
	std::atomic<size_t> m_idleThreadCount = {0};

	// (JP) 主线程是否用作工作线程
	bool m_useCaller;
	// (JP) 如果是 -> 需要额外创建调度协程
	std::shared_ptr<Fiber> m_schedulerFiber;
	// (JP) 如果是 -> 记录主线程的线程id
	int m_rootThread = -1;
	// (JP) 是否正在关闭
	bool m_stopping = false;	
};

}

#endif