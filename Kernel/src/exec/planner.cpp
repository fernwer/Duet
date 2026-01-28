#include "exec/planner.hpp"
#include "exec/type_mapper.hpp"

#include "pg_under_macro.hpp"
#include "mqo.pb.h"
#include "pg_redef_macro.hpp"

#include <stdexcept>

std::map<std::string, SPIPlanPtr> Planner::plan_cache_;

const size_t MAX_PLAN_CACHE_SIZE = 50;

Planner::Planner() {
}
Planner::~Planner() {
}

SPIPlanPtr Planner::PrepareSPI(const mqo::BatchPayload& payload) {

    if (payload.rows_size() == 0) {
        return NULL;
    }

    // Type Infer
    const auto& first_row = payload.rows(0);
    int arg_count = first_row.values_size();
    std::vector<Oid> arg_types(arg_count);

    for (int i = 0; i < arg_count; ++i) {
        arg_types[i] = TypeMapper::DeduceTypeOid(first_row.values(i));
    }

    SPIPlanPtr plan = SPI_prepare(payload.template_sql().c_str(), arg_count, arg_types.data());

    if (!plan) {
        throw std::runtime_error("SPI_prepare failed. Code: " + std::to_string(SPI_result));
    }

    return plan;
}

SPIPlanPtr Planner::PrepareMQO(const mqo::BatchPayload& payload) {
    if (payload.rows_size() == 0) return NULL;
    std::string sql_key = payload.template_sql();

    if (plan_cache_.size() >= MAX_PLAN_CACHE_SIZE) {
        elog(DEBUG1, "[Lumos] Plan cache full (%lu), flushing...", plan_cache_.size());
        for (auto& pair : plan_cache_) {
            if (pair.second != NULL) {
                SPI_freeplan(pair.second);
            }
        }
        plan_cache_.clear();
    }

    auto it = plan_cache_.find(sql_key);
    if (it != plan_cache_.end()) {
        SPIPlanPtr cached_plan = it->second;
        if (SPI_plan_is_valid(cached_plan)) {
            return cached_plan;
        } else {
            SPI_freeplan(cached_plan);
            plan_cache_.erase(it);
        }
    }

    int arg_count = payload.rows(0).values_size();
    std::vector<Oid> arg_types(arg_count);
    if (payload.param_types_size() == arg_count) {
        for (int i = 0; i < arg_count; ++i) {
            arg_types[i] = TypeMapper::ResolveTypeOid(payload.param_types(i));
        }
    } else {
        for (int i = 0; i < arg_count; ++i) {
            arg_types[i] = TypeMapper::DeduceTypeOid(payload.rows(0).values(i));
        }
    }

    SPIPlanPtr plan = SPI_prepare(sql_key.c_str(), arg_count, arg_types.data());
    if (!plan) throw std::runtime_error("SPI_prepare MQO failed.");

    if (SPI_keepplan(plan) == 0) {
        plan_cache_[sql_key] = plan;
    }
    return plan;
}