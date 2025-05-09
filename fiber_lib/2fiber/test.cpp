#include "fiber.h"
#include <vector>

using namespace sylar; 

class Scheduler
{
public:
	// コルーチンタスクを追加する
	void schedule(std::shared_ptr<Fiber> task)
	{
		m_tasks.push_back(task);
	}

	// スケジューリングタスクを実行する
	void run()
	{
		std::cout << " number " << m_tasks.size() << std::endl;

		std::shared_ptr<Fiber> task;
		auto it = m_tasks.begin();
		while(it!=m_tasks.end())
		{
				// イテレータ自体もポインタである
			task = *it;
			// メインコルーチンから子コルーチンへ切り替え、子コルーチン関数が終了したら自動的にメインコルーチンへ戻る
			task->resume();
			it++;
		}
		m_tasks.clear();
	}

private:
	// タスクキュー
	std::vector<std::shared_ptr<Fiber>> m_tasks;
};

void test_fiber(int i)
{
	std::cout << "hello world " << i << std::endl;
}

int main()
{
	// 現在のスレッドのメインコルーチンを初期化する
	Fiber::GetThis();

	// スケジューラを作成
	Scheduler sc;

	// スケジューリングタスクを追加（タスクは子コルーチンとバインドされる）
	for(auto i=0;i<20;i++)
	{
		// 子コルーチンを作成
			// 共有ポインタを使ってリソースを自動管理 -> 解放は子コルーチン終了後に自動で行われる
			// bind関数 -> 関数と引数をバインドして関数オブジェクトを返す
		std::shared_ptr<Fiber> fiber = std::make_shared<Fiber>(std::bind(test_fiber, i), 0, false);
		sc.schedule(fiber);
	}

	// スケジューリングタスクを実行する
	sc.run();

	return 0;
}