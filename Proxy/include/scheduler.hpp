#pragma once

#include "common.hpp"
#include "db_utils.hpp"

#include <map>
#include <mutex>
#include <thread>
#include <atomic>

class BatchScheduler {
public:
    BatchScheduler(size_t max_batch_size, size_t window_ms, bool dry_run, const std::string& conn_str);
    ~BatchScheduler();

    // Entry point for submission
    void Submit(int req_id, const std::string& sql);

private:
    void RunLoop();
    void FlushBatch(const QueryBatch& batch, bool use_debug_mode = false);

    std::string GenerateKernelPayload(const QueryBatch& batch, bool use_debug_func);

    std::string ToHex(const std::string& input);
    std::string GetPGTypeName(ParamType type);
    void TryExtractScanHint(const std::string& sql, std::string& out_table, std::string& out_col);

    std::atomic<bool> running_;

    size_t max_batch_size_;
    size_t window_ms_;

    bool dry_run_mode_;

    std::unique_ptr<PGConnection> db_conn_;

    std::thread worker_thread_;
    std::mutex queue_mutex_;
    // Map: Fp_Hash -> Batch
    std::map<uint64_t, QueryBatch> pending_batches_;
};
