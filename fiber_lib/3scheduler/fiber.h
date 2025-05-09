#ifndef _COROUTINE_H_
#define _COROUTINE_H_

#include <iostream>     
#include <memory>       
#include <atomic>       
#include <functional>   
#include <cassert>      
#include <ucontext.h>   
#include <unistd.h>
#include <mutex>

namespace sylar {

class Fiber : public std::enable_shared_from_this<Fiber>
{
public:
	// コルーチンの状態
	enum State
	{
		READY, 
		RUNNING, 
		TERM 
	};

private:
	// GetThis()のみが呼び出す -> private -> メインコルーチンを作成  
	Fiber();

public:
	Fiber(std::function<void()> cb, size_t stacksize = 0, bool run_in_scheduler = true);
	~Fiber();

	// コルーチンを再利用する
	void reset(std::function<void()> cb);

	// タスクスレッドが再開実行する
	void resume();
	// タスクスレッドが実行権を譲る
	void yield();

	uint64_t getId() const {return m_id;}
	State getState() const {return m_state;}

public:
	// 現在実行中のコルーチンを設定
	static void SetThis(Fiber *f);

	// 現在実行中のコルーチンを取得 
	static std::shared_ptr<Fiber> GetThis();

	// スケジューラコルーチンを設定（デフォルトはメインコルーチン）
	static void SetSchedulerFiber(Fiber* f);
	
	// 現在実行中のコルーチンを取得id
	static uint64_t GetFiberId();

	// コルーチン関数
	static void MainFunc();	

private:
	// id
	uint64_t m_id = 0;
	// スタックサイズ
	uint32_t m_stacksize = 0;
	// コルーチンの状態
	State m_state = READY;
	// コルーチンコンテキスト
	ucontext_t m_ctx;
	// コルーチンスタックポインタ
	void* m_stack = nullptr;
	// コルーチン関数
	std::function<void()> m_cb;
	// スケジューラコルーチンに実行権を渡すかどうか
	bool m_runInScheduler;

public:
	std::mutex m_mutex;
};

}

#endif

