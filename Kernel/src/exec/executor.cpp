#include "exec/executor.hpp"
#include "pg_under_macro.hpp"
#include "mqo.pb.h"
#include <stdexcept>

Executor::Executor() {
    planner_ = std::make_unique<Planner>();
    runtime_ = std::make_unique<Runtime>();
}
Executor::~Executor() {
}

int Executor::Execute(const mqo::BatchPayload& payload) {

    if (!payload.scan_table().empty() && !payload.scan_col().empty()) {
        return DispatchSharedScan(payload);
    }

    if (payload.use_mqo()) {
        return DispatchMQO(payload);
    }

    return DispatchStandard(payload);
}

int Executor::DispatchStandard(const mqo::BatchPayload& payload) {
    int ret = SPI_connect();
    if (ret != SPI_OK_CONNECT) throw std::runtime_error("SPI Connect failed");
    int res = 0;
    try {
        SPIPlanPtr plan = planner_->PrepareSPI(payload);
        if (plan) {
            res = runtime_->ExecuteSPILoop(plan, payload);
            SPI_freeplan(plan);
        }
    } catch (...) {
        SPI_finish();
        throw;
    }
    SPI_finish();
    return res;
}

int Executor::DispatchMQO(const mqo::BatchPayload& payload) {
    int ret = SPI_connect();
    if (ret != SPI_OK_CONNECT) throw std::runtime_error("SPI Connect failed");
    int res = 0;
    try {
        SPIPlanPtr plan = planner_->PrepareMQO(payload);
        if (plan) {
            res = runtime_->ExecuteBatchMQO(plan, payload);
        }
    } catch (...) {
        SPI_finish();
        throw;
    }
    SPI_finish();
    return res;
}

int Executor::DispatchSharedScan(const mqo::BatchPayload& payload) {
    int ret = SPI_connect();
    if (ret != SPI_OK_CONNECT) throw std::runtime_error("SPI Connect failed");
    int res = 0;
    try {
        res = runtime_->ExecuteSharedScan(payload);
    } catch (...) {
        SPI_finish();
        throw;
    }
    SPI_finish();
    return res;
}