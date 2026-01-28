#include "lumos_kernel.hpp"
#include "pg_under_macro.hpp"
#include "mqo.pb.h"
#include "pg_redef_macro.hpp"
#include <sstream>

LumosKernel::LumosKernel() {
    executor_ = std::make_unique<Executor>();
}
LumosKernel::~LumosKernel() {
}

void LumosKernel::Dispatch(const char* data, size_t len) {
    mqo::BatchPayload payload;
    if (!payload.ParseFromArray(data, len)) {
        elog(ERROR, "LumosKernel: Protobuf parsing failed.");
        return;
    }

    if (payload.dry_run()) {
        elog(DEBUG1, "[Lumos] Mode: Dry-Run (Sandboxed Execution)");
    } else if (!payload.scan_table().empty()) {
        elog(DEBUG1, "[Lumos] Mode: Shared Scan on %s", payload.scan_table().c_str());
    }

    try {
        int count = executor_->Execute(payload);
        elog(DEBUG1, "[Lumos] Batch completed. Processed/Simulated %d rows.", count);
    } catch (const std::exception& e) {
        elog(ERROR, "LumosKernel Exception: %s", e.what());
    }
}

std::string LumosKernel::DebugAnalyze(const char* data, size_t len) {
    mqo::BatchPayload payload;
    if (!payload.ParseFromArray(data, len)) return "Parse Error";

    std::stringstream ss;
    ss << "SQL: " << payload.template_sql() << "\n"
       << "Rows: " << payload.rows_size() << "\n"
       << "DryRun: " << (payload.dry_run() ? "YES" : "NO") << "\n";

    ss << "Params: [";
    for (int i = 0; i < payload.param_types_size(); ++i) {
        ss << payload.param_types(i) << (i < payload.param_types_size() - 1 ? ", " : "");
    }
    ss << "]\n";

    if (!payload.scan_table().empty()) {
        ss << "ScanHint: Table=" << payload.scan_table() << ", Col=" << payload.scan_col();
    } else {
        ss << "ScanHint: NONE";
    }

    return ss.str();
}