#include <iostream>
#include <sstream>
#include <vector>
#include <regex>

#include "scheduler.hpp"
#include "pg_query.h"
#include "parser.hpp"
#include "mqo.pb.h"

BatchScheduler::BatchScheduler(size_t max_batch_size, size_t window_ms, bool dry_run, const std::string& conn_str)
    : max_batch_size_(max_batch_size), window_ms_(window_ms), dry_run_mode_(dry_run), running_(true) {

    try {
        db_conn_ = std::make_unique<PGConnection>(conn_str);
    } catch (const std::exception& e) {
        std::cerr << "[FATAL] DB Connect Error: " << e.what() << std::endl;
        exit(1);
    }

    worker_thread_ = std::thread(&BatchScheduler::RunLoop, this);
}

BatchScheduler::~BatchScheduler() {
    running_ = false;
    if (worker_thread_.joinable()) worker_thread_.join();
}

void BatchScheduler::Submit(int req_id, const std::string& sql) {
    ParsedQuery parsed;
    if (!SQLParser::Analyze(req_id, sql, parsed)) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        auto& batch = pending_batches_[parsed.fp_hash];
        if (batch.queries.empty()) {
            batch.fingerprint = parsed.fingerprint;
            batch.fp_hash = parsed.fp_hash;
        }
        batch.queries.push_back(parsed);
        if (batch.queries.size() >= max_batch_size_) {
            FlushBatch(batch);
            pending_batches_.erase(parsed.fp_hash);
        }
    }
}

void BatchScheduler::RunLoop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(window_ms_));
        std::lock_guard<std::mutex> lock(queue_mutex_);
        for (auto it = pending_batches_.begin(); it != pending_batches_.end();) {
            auto& batch = it->second;
            if (!batch.queries.empty()) {
                FlushBatch(batch);
            }
            it = pending_batches_.erase(it);
        }
    }
}

void BatchScheduler::FlushBatch(const QueryBatch& batch, bool use_debug_mode) {
    if (batch.queries.empty()) return;

    use_debug_mode = true;

    std::string sql = GenerateKernelPayload(batch, use_debug_mode);

#if 1
    std::cout << "[Proxy] Dispatching Batch (Hash=" << batch.fp_hash << ", Size=" << batch.queries.size() << ")..."
              << std::endl;
#endif

    try {
        std::string result = db_conn_->ExecuteScalar(sql);

        if (use_debug_mode) {
            std::cout << "\n========== [KERNEL DEBUG REPORT] ==========\n";
            std::cout << result << std::endl;
            std::cout << "===========================================\n" << std::endl;
        } else {
            std::cout << "[Proxy] Batch executed successfully." << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "[Proxy] Batch Execution Failed: " << e.what() << std::endl;
    }
}

std::string BatchScheduler::GetPGTypeName(ParamType type) {
    switch (type) {
        case ParamType::INTEGER: return "int8";
        case ParamType::FLOAT: return "float8";
        case ParamType::BOOL: return "bool";
        case ParamType::STRING: return "text";
        case ParamType::NULL_VAL: return "text";
        default: return "text";
    }
}

void BatchScheduler::TryExtractScanHint(const std::string& sql, std::string& out_table, std::string& out_col) {
    static const std::regex re_hint(R"(FROM\s+([a-zA-Z0-9_]+)\s+WHERE\s+([a-zA-Z0-9_]+)\s*=)", std::regex::icase);

    std::smatch match;
    if (std::regex_search(sql, match, re_hint)) {
        if (match.size() == 3) {
            out_table = match[1].str();
            out_col = match[2].str();
        }
    }
}

std::string BatchScheduler::GenerateKernelPayload(const QueryBatch& batch, bool use_debug_func) {
    mqo::BatchPayload proto_payload;

    std::string base_sql = batch.queries[0].original_sql;
    PgQueryNormalizeResult norm_result = pg_query_normalize(base_sql.c_str());

    std::string final_template;
    if (norm_result.error) {
        final_template = base_sql;
        pg_query_free_normalize_result(norm_result);
    } else {
        final_template = norm_result.normalized_query;
        pg_query_free_normalize_result(norm_result);
    }
    proto_payload.set_template_sql(final_template);

    proto_payload.set_use_mqo(true);
    proto_payload.set_dry_run(dry_run_mode_);

    std::string hint_table, hint_col;
    TryExtractScanHint(base_sql, hint_table, hint_col);
    if (!hint_table.empty() && !hint_col.empty()) {
        proto_payload.set_scan_table(hint_table);
        proto_payload.set_scan_col(hint_col);
    }

    if (!batch.queries.empty()) {
        const auto& first_query = batch.queries[0];
        for (const auto& p : first_query.params) {
            proto_payload.add_param_types(GetPGTypeName(p.type));
        }
    }

    for (const auto& query : batch.queries) {
        mqo::ParamRow* row = proto_payload.add_rows();
        for (const auto& p : query.params) {
            mqo::Value* val = row->add_values();
            val->set_is_null(false); 
            switch (p.type) {
                case ParamType::INTEGER: try { val->set_int_val(std::stoll(p.value));
                    } catch (...) {
                        val->set_int_val(0);
                    }
                    break;
                case ParamType::FLOAT: try { val->set_float_val(std::stod(p.value));
                    } catch (...) {
                        val->set_float_val(0.0);
                    }
                    break;
                case ParamType::BOOL: val->set_bool_val((p.value == "true" || p.value == "t")); break;
                case ParamType::NULL_VAL: val->set_is_null(true); break;
                default: val->set_string_val(p.value); break;
            }
        }
    }

    std::string binary_data;
    if (!proto_payload.SerializeToString(&binary_data)) return "";

    std::stringstream ss;

    if (use_debug_func) {
        ss << "SELECT mqo_debug(decode('" << ToHex(binary_data) << "', 'hex'));";
    } else {
        ss << "SELECT mqo_dispatch(decode('" << ToHex(binary_data) << "', 'hex'));";
    }

    return ss.str();
}

std::string BatchScheduler::ToHex(const std::string& input) {
    static const char hex_digits[] = "0123456789ABCDEF";
    std::string output;
    output.reserve(input.length() * 2);
    for (unsigned char c : input) {
        output.push_back(hex_digits[c >> 4]);
        output.push_back(hex_digits[c & 0x0F]);
    }
    return output;
}