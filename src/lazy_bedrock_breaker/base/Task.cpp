#include "Task.h"

namespace lazy_bedrock_breaker {
std::vector<Task> TaskManager::tasks;

void TaskManager::addTask(const std::function<bool()>& action) {
    tasks.push_back(Task{action});
    // 做一个插值，将任务插入到正确的位置，保持tasks按照delay从小到大排序
}

void TaskManager::tick() {
    static std::vector<Task> newTasks;
    newTasks.clear();

    // 交换 tasks，这样在执行任务过程中如果调用了 addTask，新任务会进到空的 tasks 里
    std::vector<Task> currentTasks;
    currentTasks.swap(tasks);

    for (auto it = currentTasks.begin(); it != currentTasks.end(); ) {
        if (it->action && it->action()) {
            it = currentTasks.erase(it);
        } else {
            ++it;
        }
    }

    // 将没执行完的任务加回到任务列表开头 (保持顺序)
    tasks.insert(tasks.begin(), currentTasks.begin(), currentTasks.end());
}
} // namespace lazy_bedrock_breaker
  