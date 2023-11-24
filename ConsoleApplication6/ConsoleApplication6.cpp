#include <iostream>
#include <vector>
#include <thread>
#include <map>
#include <mutex>
#include <condition_variable>
#include <syncstream>
#include <algorithm>

void f(char set, int action) {
    std::osyncstream(std::cout) << "Action " << action << " made from the group " << set << ".\n";
}

struct Task {
    char set;
    int count;
    std::vector<char> dependencies;
    int remainingDependencies;
    bool isCompleted;

    // Додати конструктор за замовчуванням
    Task() : set(0), count(0), remainingDependencies(0), isCompleted(false) {}

    // Конструктор з параметрами
    Task(char s, int c, std::vector<char> d, int r, bool ic)
        : set(s), count(c), dependencies(d), remainingDependencies(r), isCompleted(ic) {}
};


std::map<char, Task> tasks;
std::mutex taskMutex;
std::condition_variable cv;
bool isComputationDone = false;
const int nt = 6;

void executeTask(char taskSet) {
    for (int i = 1; i <= tasks[taskSet].count; ++i) {
        f(taskSet, i);
    }
}

void threadFunc() {
    while (true) {
        std::unique_lock<std::mutex> lock(taskMutex);

        cv.wait(lock, [] {
            return std::any_of(tasks.begin(), tasks.end(), [](const auto& task) {
                return task.second.remainingDependencies == 0 && !task.second.isCompleted;
                }) || isComputationDone;
            });

        if (isComputationDone) {
            break;
        }

        char taskToExecute = 0;
        for (auto& task : tasks) {
            if (task.second.remainingDependencies == 0 && !task.second.isCompleted) {
                taskToExecute = task.first;
                task.second.isCompleted = true;
                break;
            }
        }

        lock.unlock();

        if (taskToExecute != 0) {
            executeTask(taskToExecute);

            lock.lock();
            for (auto& t : tasks) {
                auto it = std::find(t.second.dependencies.begin(), t.second.dependencies.end(), taskToExecute);
                if (it != t.second.dependencies.end()) {
                    t.second.remainingDependencies--;
                }
            }

            cv.notify_all();
        }

        if (std::all_of(tasks.begin(), tasks.end(), [](const auto& task) {
            return task.second.isCompleted;
            })) {
            isComputationDone = true;
            cv.notify_all();
            break;
        }
    }
}

int main() {
    std::osyncstream(std::cout) << "Processing started. \n";

    // Ініціалізація завдань згідно з новими значеннями та схемою залежностей
    tasks = {
        {'a', Task('a', 6, {}, 0, false)},
        {'b', Task('b', 7, {}, 0, false)},
        {'c', Task('c', 9, {}, 0, false)},
        {'d', Task('d', 8, {}, 0, false)},
        {'e', Task('e', 5, {}, 0, false)},
        {'f', Task('f', 9, {}, 0, false)},
        {'g', Task('g', 7, {'a', 'e'}, 2, false)},
        {'h', Task('h', 4, {'b', 'c'}, 2, false)},
        {'i', Task('i', 9, {'d', 'f'}, 2, false)},
        {'j', Task('j', 6, {'g', 'h', 'i'}, 3, false)}
    };

    for (auto& task : tasks) {
        task.second.remainingDependencies = task.second.dependencies.size();
    }

    std::vector<std::thread> threads;
    for (int i = 0; i < nt; ++i) {
        threads.emplace_back(threadFunc);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    std::osyncstream(std::cout) << "Processing is complete.  \n";
    return 0;
}
