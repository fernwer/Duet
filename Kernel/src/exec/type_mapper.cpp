#include "exec/type_mapper.hpp"

#include "pg_under_macro.hpp"
#include "mqo.pb.h"
#include "pg_redef_macro.hpp"

#include <cstring>

PgParam TypeMapper::ToPgParam(const mqo::Value& val, Oid target_type) {
    PgParam p;
    p.type_id = target_type;
    p.null_flag = ' ';

    if (val.is_null()) {
        p.value = (Datum)0;
        p.null_flag = 'n';
        return p;
    }

    switch (val.typed_value_case()) {
        case mqo::Value::kIntVal: p.value = Int64GetDatum(val.int_val()); break;
        case mqo::Value::kFloatVal: p.value = Float8GetDatum(val.float_val()); break;
        case mqo::Value::kStringVal:
            p.value = CStringGetTextDatum(val.string_val().c_str());
            break;
        case mqo::Value::kBoolVal: p.value = BoolGetDatum(val.bool_val()); break;
        default:
            p.value = (Datum)0;
            p.null_flag = 'n';
            break;
    }
    return p;
}

Oid TypeMapper::DeduceTypeOid(const mqo::Value& val) {
    switch (val.typed_value_case()) {
        case mqo::Value::kIntVal: return INT8OID;
        case mqo::Value::kFloatVal: return FLOAT8OID;
        case mqo::Value::kStringVal: return TEXTOID;
        case mqo::Value::kBoolVal: return BOOLOID;
        default: return TEXTOID; // Fallback
    }
}

Oid TypeMapper::ResolveTypeOid(const std::string& type_name) {
    if (type_name.empty()) return TEXTOID;
    Oid type_oid = InvalidOid;
    try {
        Datum name_datum = CStringGetDatum(type_name.c_str());
        type_oid = DatumGetObjectId(DirectFunctionCall1(regtypein, name_datum));
    } catch (...) {
        type_oid = TEXTOID;
    }
    return type_oid;
}

bool TypeMapper::ValueEqual(Oid type_oid, Datum a, Datum b) {
    int16 typlen;
    bool typbyval;
    get_typlenbyval(type_oid, &typlen, &typbyval);
    if (typbyval) {
        return a == b;
    } else {
        return datumIsEqual(a, b, typbyval, typlen);
    }
}