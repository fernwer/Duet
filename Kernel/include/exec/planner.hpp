#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>

extern "C" {
#include "postgres.h"
#include "executor/spi.h"
}

namespace mqo {
class BatchPayload;
}

class Planner {
public:
    Planner();
    ~Planner();

    SPIPlanPtr PrepareSPI(const mqo::BatchPayload& payload); // Basic SPI func for batch SQL exec.
    SPIPlanPtr PrepareMQO(const mqo::BatchPayload& payload); // MQO Cache mode.

private:
    static std::map<std::string, SPIPlanPtr> plan_cache_;
};