#include <deletion_queue.h>
#include <fmt/core.h>

void DeletionQueue::push_function(std::function<void()>&& function, const std::string & name) {
    deletors.emplace_back(std::move(function), name);
}

void DeletionQueue::flush(){
    // reverse iterate the deletion queue to execute all the functions
    for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
        const std::string &name = it->second;
        if (!name.empty()) {
            fmt::print("Running deletor: {}\n", name);
        } else {
            fmt::print("Running unnamed deletor\n");
        }

        (*it).first(); //call functors
    }

    deletors.clear();
}