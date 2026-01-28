#include <iostream>
#include <algorithm>
#include <unordered_set>

#include "parser.hpp"
#include "pg_query.h"

// Protobuf based Parser (faster)
#include "pg_query_cpp.pb.h"

#define TOKEN_MINUS 45

bool SQLParser::Analyze(int req_id, const std::string& sql, ParsedQuery& out_result) {
    auto start_time = std::chrono::high_resolution_clock::now();

    PgQueryFingerprintResult fp_result = pg_query_fingerprint(sql.c_str());
    if (fp_result.error) {
        pg_query_free_fingerprint_result(fp_result);
        return false;
    }

    out_result.fp_hash = fp_result.fingerprint;
    out_result.request_id = req_id;
    out_result.original_sql = sql;
    out_result.arrive_time = start_time;
    pg_query_free_fingerprint_result(fp_result);

    PgQueryScanResult scan_result = pg_query_scan(sql.c_str());
    if (scan_result.error) {
        pg_query_free_scan_result(scan_result);
        return false;
    }

    pg_query_cpp::ScanResult scan_proto;

    bool parse_success = scan_proto.ParseFromArray(scan_result.pbuf.data, scan_result.pbuf.len);

    pg_query_free_scan_result(scan_result);

    if (!parse_success) {
        return false;
    }

    out_result.params.reserve(scan_proto.tokens_size() / 4);

    for (int i = 0; i < scan_proto.tokens_size(); ++i) {
        const auto& token = scan_proto.tokens(i);
        const int kind = token.token();

        QueryParam param;
        bool extracted = false;

        switch (kind) {
            case pg_query_cpp::ICONST:
            case pg_query_cpp::FCONST: {
                param.type = (kind == pg_query_cpp::ICONST) ? ParamType::INTEGER : ParamType::FLOAT;

                bool is_negative = false;
                if (i > 0) {
                    const auto& prev = scan_proto.tokens(i - 1);
                    if (prev.token() == TOKEN_MINUS && prev.end() == token.start()) {
                        is_negative = true;
                    }
                }
                std::string val = sql.substr(token.start(), token.end() - token.start());
                param.value = is_negative ? ("-" + val) : std::move(val);
                extracted = true;
                break;
            }
            case pg_query_cpp::SCONST: {
                param.type = ParamType::STRING;
                int len = token.end() - token.start();
                if (len >= 2)
                    param.value = sql.substr(token.start() + 1, len - 2);
                else
                    param.value = "";
                extracted = true;
                break;
            }
            case pg_query_cpp::BCONST:
            case pg_query_cpp::XCONST: {
                param.type = ParamType::STRING;
                param.value = sql.substr(token.start(), token.end() - token.start());
                extracted = true;
                break;
            }
            case pg_query_cpp::TRUE_P: {
                param.type = ParamType::BOOL;
                param.value = "true";
                extracted = true;
                break;
            }
            case pg_query_cpp::FALSE_P: {
                param.type = ParamType::BOOL;
                param.value = "false";
                extracted = true;
                break;
            }
            case pg_query_cpp::NULL_P: {
                param.type = ParamType::NULL_VAL;
                param.value = "null";
                extracted = true;
                break;
            }
            default: break;
        }

        if (extracted) {
            out_result.params.push_back(std::move(param));
        }
    }

    return true;
}