#pragma once

#include <functional>
#include <vector>

namespace lazy_bedrock_breaker {
struct Task {
    std::function<void()> function; // 需要执行的函数
    int delay; // 延迟的时间，单位ticks
};

class TaskManager {
private:
    static std::vector<Task> tasks; // 存储所有的任务

public:
    static void addTask(const std::function<void()>& func, int delay);

    static void tick();
};
}