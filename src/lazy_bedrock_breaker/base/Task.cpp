#include "Task.h"

namespace lazy_bedrock_breaker {
std::vector<Task> TaskManager::tasks;

void TaskManager::addTask(const std::function<void()>& func, int delay) {
    // 做一个插值，将任务插入到正确的位置，保持tasks按照delay从小到大排序
    auto it = tasks.begin();
    while (it != tasks.end() && it->delay <= delay) {
        ++it;
    }
    tasks.insert(it, Task{func, delay});
}

void TaskManager::tick() {
// 每tick调用一次，执行所有delay为0的任务，并将其他任务的delay减1
while (!tasks.empty() && tasks.front().delay <= 0) {
    tasks.front().function(); // 执行任务
    tasks.erase(tasks.begin()); // 移除已执行的任务
}
for (auto& task : tasks) {
    task.delay--; // 将剩余任务的delay减1
}
}
} // namespace lazy_bedrock_breaker
  