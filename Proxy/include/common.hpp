#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Using Hash Func for Efficient Fingerprint Compare
struct HashUtils {
    // Using FNV-1a 64-bit Hash Alg
    static inline uint64_t Compute(const std::string& str){
        uint64_t hash = 0xcbf29ce484222325ULL;
        for (char c:str){
            hash ^= static_cast<uint64_t>(c);
            hash *= 0x100000001b3ULL;
        }
        return hash;
    }
};

// SQL Params Type Enum
enum class ParamType {
    INTEGER,
    FLOAT,
    STRING,
    BOOL,
    NULL_VAL,
    UNKNOWN
};

// Extracted Single Param
struct QueryParam {
    ParamType type;
    std::string value; // Unified store as string for subsequent serialization
};

// Parsed Query Object
struct ParsedQuery{
    int request_id;
    std::string original_sql;
    std::string fingerprint; // AST string after masking (core Grouping Key)
    uint64_t fp_hash;
    std::vector<QueryParam> params; // Ordered list of extracted params
    std::chrono::high_resolution_clock::time_point arrive_time;
};

// A batch prepared to be sent to the db kernel
struct QueryBatch{
    std::string fingerprint;
    uint64_t fp_hash;
    std::vector<ParsedQuery> queries;
};
