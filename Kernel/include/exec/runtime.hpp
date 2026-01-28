#pragma once

#include <vector>

extern "C" {
#include "postgres.h"
#include "executor/spi.h"
#include "utils/memutils.h"
#include "utils/elog.h"
#include "access/xact.h"
#include "access/table.h"
#include "access/tableam.h"
#include "access/htup_details.h"
#include "utils/snapmgr.h"
#include "utils/lsyscache.h"
#include "utils/rel.h"
}

namespace mqo {
class BatchPayload;
}

class Runtime {
public:
    Runtime();
    ~Runtime();

    // Loop execution
    int ExecuteSPILoop(SPIPlanPtr plan, const mqo::BatchPayload& payload);

    // [MQO Core] Context Reuse + Snapshot Reuse + Dry-Run Support
    int ExecuteBatchMQO(SPIPlanPtr plan, const mqo::BatchPayload& payload);

    // [IO Optimization] Shared Scan
    int ExecuteSharedScan(const mqo::BatchPayload& payload);

private:
    static MemoryContext mqo_session_context_;
};