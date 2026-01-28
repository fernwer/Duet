#include "lumos_kernel.hpp"

extern "C" {
#include "postgres.h"
#include "fmgr.h"
#include "utils/bytea.h"
#include "utils/builtins.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

PG_FUNCTION_INFO_V1(mqo_dispatch);
Datum mqo_dispatch(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(mqo_debug);
Datum mqo_debug(PG_FUNCTION_ARGS);
}

Datum mqo_dispatch(PG_FUNCTION_ARGS) {
    bytea* data_ptr = PG_GETARG_BYTEA_P(0);
    size_t data_len = VARSIZE_ANY_EXHDR(data_ptr);
    const char* data_content = VARDATA_ANY(data_ptr);

    try {
        LumosKernel kernel;
        kernel.Dispatch(data_content, data_len);
        PG_RETURN_INT32(1);
    } catch (...) {
        ereport(ERROR, (errmsg("[Lumos] Critical Dispatch Error.")));
    }
    PG_RETURN_INT32(0);
}

Datum mqo_debug(PG_FUNCTION_ARGS) {
    bytea* data_ptr = PG_GETARG_BYTEA_P(0);
    size_t data_len = VARSIZE_ANY_EXHDR(data_ptr);
    const char* data_content = VARDATA_ANY(data_ptr);

    LumosKernel kernel;
    std::string report = kernel.DebugAnalyze(data_content, data_len);
    PG_RETURN_TEXT_P(cstring_to_text(report.c_str()));
}