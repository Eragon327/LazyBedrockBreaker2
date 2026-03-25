#pragma once

#include <functional>
#include <vector>

#define MAX_TASK_RUN_TIMES 10 // 定义一个最大运行次数，防止死循环

namespace lazy_bedrock_breaker {
struct Task {
    std::function<bool()> action; // 需要执行的函数
};

class TaskManager {
private:
    static std::vector<Task> tasks; // 存储所有的任务

public:
    static void
    addTask(const std::function<bool()>& action);

    static void tick();
};
}