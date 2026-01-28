#pragma once

#include <memory>

#include "planner.hpp"
#include "runtime.hpp"

namespace mqo {
class BatchPayload;
}

class Executor {
public:
    Executor();
    ~Executor();

    int Execute(const mqo::BatchPayload& payload);

private:
    int DispatchStandard(const mqo::BatchPayload& payload);
    int DispatchMQO(const mqo::BatchPayload& payload);
    int DispatchSharedScan(const mqo::BatchPayload& payload);

    std::unique_ptr<Planner> planner_;
    std::unique_ptr<Runtime> runtime_;
};