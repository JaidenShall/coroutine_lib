#include "fiber.h"

static bool debug = false;

namespace sylar {

// 現在のスレッド上のコルーチン制御情報

// 実行中のコルーチン
static thread_local Fiber* t_fiber = nullptr;
// メインコルーチン
static thread_local std::shared_ptr<Fiber> t_thread_fiber = nullptr;
// スケジューラコルーチン
static thread_local Fiber* t_scheduler_fiber = nullptr;

// コルーチンカウンタ
static std::atomic<uint64_t> s_fiber_id{0};
// コルーチンID
static std::atomic<uint64_t> s_fiber_count{0};

void Fiber::SetThis(Fiber *f)
{
	t_fiber = f;
}

// 最初にこの関数を実行してメインコルーチンを作成する
std::shared_ptr<Fiber> Fiber::GetThis()
{
	if(t_fiber)
	{	
		return t_fiber->shared_from_this();
	}

	std::shared_ptr<Fiber> main_fiber(new Fiber());
	t_thread_fiber = main_fiber;
	t_scheduler_fiber = main_fiber.get(); // 明示的に設定しない限り、メインコルーチンはデフォルトでスケジューラになる
	
	assert(t_fiber == main_fiber.get());
	return t_fiber->shared_from_this();
}

void Fiber::SetSchedulerFiber(Fiber* f)
{
	t_scheduler_fiber = f;
}

uint64_t Fiber::GetFiberId()
{
	if(t_fiber)
	{
		return t_fiber->getId();
	}
	return (uint64_t)-1;
}

Fiber::Fiber()
{
	SetThis(this);
	m_state = RUNNING;
	
	if(getcontext(&m_ctx))
	{
		std::cerr << "Fiber() failed\n";
		pthread_exit(NULL);
	}
	
	m_id = s_fiber_id++;
	s_fiber_count ++;
	if(debug) std::cout << "Fiber(): main id = " << m_id << std::endl;
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool run_in_scheduler):
m_cb(cb), m_runInScheduler(run_in_scheduler)
{
	m_state = READY;

	// コルーチンのスタック領域を割り当てる
	m_stacksize = stacksize ? stacksize : 128000;
	m_stack = malloc(m_stacksize);

	if(getcontext(&m_ctx))
	{
		std::cerr << "Fiber(std::function<void()> cb, size_t stacksize, bool run_in_scheduler) failed\n";
		pthread_exit(NULL);
	}
	
	m_ctx.uc_link = nullptr;
	m_ctx.uc_stack.ss_sp = m_stack;
	m_ctx.uc_stack.ss_size = m_stacksize;
	makecontext(&m_ctx, &Fiber::MainFunc, 0);
	
	m_id = s_fiber_id++;
	s_fiber_count ++;
	if(debug) std::cout << "Fiber(): child id = " << m_id << std::endl;
}

Fiber::~Fiber()
{
	s_fiber_count --;
	if(m_stack)
	{
		free(m_stack);
	}
	if(debug) std::cout << "~Fiber(): id = " << m_id << std::endl;	
}

void Fiber::reset(std::function<void()> cb)
{
	assert(m_stack != nullptr&&m_state == TERM);

	m_state = READY;
	m_cb = cb;

	if(getcontext(&m_ctx))
	{
		std::cerr << "reset() failed\n";
		pthread_exit(NULL);
	}

	m_ctx.uc_link = nullptr;
	m_ctx.uc_stack.ss_sp = m_stack;
	m_ctx.uc_stack.ss_size = m_stacksize;
	makecontext(&m_ctx, &Fiber::MainFunc, 0);
}

void Fiber::resume()
{
	assert(m_state==READY);
	
	m_state = RUNNING;

	if(m_runInScheduler)
	{
		SetThis(this);
		if(swapcontext(&(t_scheduler_fiber->m_ctx), &m_ctx))
		{
			std::cerr << "resume() to t_scheduler_fiber failed\n";
			pthread_exit(NULL);
		}		
	}
	else
	{
		SetThis(this);
		if(swapcontext(&(t_thread_fiber->m_ctx), &m_ctx))
		{
			std::cerr << "resume() to t_thread_fiber failed\n";
			pthread_exit(NULL);
		}	
	}
}

void Fiber::yield()
{
	assert(m_state==RUNNING || m_state==TERM);

	if(m_state!=TERM)
	{
		m_state = READY;
	}

	if(m_runInScheduler)
	{
		SetThis(t_scheduler_fiber);
		if(swapcontext(&m_ctx, &(t_scheduler_fiber->m_ctx)))
		{
			std::cerr << "yield() to to t_scheduler_fiber failed\n";
			pthread_exit(NULL);
		}		
	}
	else
	{
		SetThis(t_thread_fiber.get());
		if(swapcontext(&m_ctx, &(t_thread_fiber->m_ctx)))
		{
			std::cerr << "yield() to t_thread_fiber failed\n";
			pthread_exit(NULL);
		}	
	}	
}

void Fiber::MainFunc()
{
	std::shared_ptr<Fiber> curr = GetThis();
	assert(curr!=nullptr);

	curr->m_cb(); 
	curr->m_cb = nullptr;
	curr->m_state = TERM;

	// 実行完了 -> 実行権を譲る
	auto raw_ptr = curr.get();
	curr.reset(); 
	raw_ptr->yield(); 
}

}