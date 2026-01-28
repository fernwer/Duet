#include "exec/runtime.hpp"
#include "exec/type_mapper.hpp"

#include "pg_under_macro.hpp"
#include "mqo.pb.h"
#include "pg_redef_macro.hpp"

#include <malloc.h>

MemoryContext Runtime::mqo_session_context_ = NULL;

Runtime::Runtime() {
}
Runtime::~Runtime() {
}

int Runtime::ExecuteSPILoop(SPIPlanPtr plan, const mqo::BatchPayload& payload) {
    if (plan == NULL) return 0;
    int success_count = 0;
    int arg_count = payload.rows(0).values_size();
    std::vector<Datum> values(arg_count);
    std::vector<char> nulls(arg_count);

    for (const auto& row : payload.rows()) {
        for (int i = 0; i < arg_count; ++i) {
            PgParam p = TypeMapper::ToPgParam(row.values(i), INT8OID);
            values[i] = p.value;
            nulls[i] = p.null_flag;
        }
        int ret = SPI_execute_plan(plan, values.data(), nulls.data(), false, 0);
        if (ret >= 0) {
            success_count++;
            SPI_freetuptable(SPI_tuptable);
        }
    }
    return success_count;
}

int Runtime::ExecuteBatchMQO(SPIPlanPtr plan, const mqo::BatchPayload& payload) {
    if (plan == NULL || payload.rows_size() == 0) return 0;

    int success_count = 0;
    int arg_count = SPI_getargcount(plan);

    std::vector<Oid> arg_types(arg_count);
    for (int i = 0; i < arg_count; ++i) arg_types[i] = SPI_getargtypeid(plan, i);

    std::vector<Datum> values(arg_count);
    std::vector<char> nulls(arg_count);

    BeginInternalSubTransaction(NULL);
    PushActiveSnapshot(GetTransactionSnapshot());

    if (mqo_session_context_ == NULL) {
        mqo_session_context_ = AllocSetContextCreate(TopMemoryContext, "LumosSessionContext", ALLOCSET_DEFAULT_SIZES);
    } else {
        MemoryContextReset(mqo_session_context_);
    }

    MemoryContext old_ctx;
    bool read_only_mode = false;
    bool error_occurred = false;

    try {
        for (const auto& row : payload.rows()) {
            if (row.values_size() != arg_count) continue;

            old_ctx = MemoryContextSwitchTo(mqo_session_context_);

            for (int i = 0; i < arg_count; ++i) {
                PgParam p = TypeMapper::ToPgParam(row.values(i), arg_types[i]);
                values[i] = p.value;
                nulls[i] = p.null_flag;
            }

            int ret = SPI_execute_plan(plan, values.data(), nulls.data(), read_only_mode, 0);

            if (ret >= 0) {
                success_count++;
                SPI_freetuptable(SPI_tuptable);
            } else {
                error_occurred = true;
            }

            MemoryContextSwitchTo(old_ctx);

            MemoryContextReset(mqo_session_context_);
        }
    } catch (...) {
        error_occurred = true;
    }

    PopActiveSnapshot();

    if (payload.dry_run() || error_occurred) {
        RollbackAndReleaseCurrentSubTransaction();
        if (payload.dry_run()) {
            elog(DEBUG1, "[Lumos Dry-Run] Simulated %d ops.", success_count);
        }
    } else {
        ReleaseCurrentSubTransaction();
    }

    malloc_trim(0);

    return success_count;
}

int Runtime::ExecuteSharedScan(const mqo::BatchPayload& payload) {
    if (payload.scan_table().empty() || payload.scan_col().empty()) return 0;

    std::string table_name = payload.scan_table();
    std::string col_name = payload.scan_col();
    int match_count = 0;

    Oid table_oid = InvalidOid;
    try {
        table_oid = DatumGetObjectId(DirectFunctionCall1(regclassin, CStringGetDatum(table_name.c_str())));
    } catch (...) {
        elog(WARNING, "SharedScan: Table '%s' not found.", table_name.c_str());
        return 0;
    }

    AttrNumber att_num = get_attnum(table_oid, col_name.c_str());
    if (att_num == InvalidAttrNumber) {
        elog(WARNING, "SharedScan: Column '%s' not found.", col_name.c_str());
        return 0;
    }

    Oid type_id = get_atttype(table_oid, att_num);
    int16 typlen;
    bool typbyval;
    get_typlenbyval(type_id, &typlen, &typbyval);

    std::vector<Datum> search_keys;
    for (const auto& row : payload.rows()) {
        if (row.values_size() > 0) {
            PgParam p = TypeMapper::ToPgParam(row.values(0), type_id);
            if (p.null_flag != 'n') {
                search_keys.push_back(p.value);
            }
        }
    }

    Relation rel = table_open(table_oid, AccessShareLock);
    Snapshot snapshot = GetTransactionSnapshot(); 
    TableScanDesc scan = table_beginscan(rel, snapshot, 0, NULL);
    TupleTableSlot* slot = table_slot_create(rel, NULL);

    while (table_scan_getnextslot(scan, ForwardScanDirection, slot)) {
        bool isnull;
        Datum val = slot_getattr(slot, att_num, &isnull);

        if (isnull) continue;

        for (Datum key : search_keys) {
            bool equal = false;
            if (typbyval) {
                if (val == key) equal = true;
            } else {
                if (datumIsEqual(val, key, typbyval, typlen)) equal = true;
            }

            if (equal) {
                match_count++;
                break;
            }
        }
    }

    ExecDropSingleTupleTableSlot(slot);
    table_endscan(scan);
    table_close(rel, AccessShareLock);

    return match_count;
}
