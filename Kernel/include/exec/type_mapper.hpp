#pragma once

#include <string>
#include <vector>

namespace mqo {
class Value;
}

extern "C" {
#include "postgres.h"
#include "catalog/pg_type.h"
#include "utils/builtins.h"
#include "parser/parse_type.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"
#include "utils/datum.h"
}

struct PgParam {
    Oid type_id;    
    Datum value;    
    char null_flag; 
};

class TypeMapper {
public:
    static PgParam ToPgParam(const mqo::Value &val, Oid target_type);

    static Oid DeduceTypeOid(const mqo::Value &val);

    static Oid ResolveTypeOid(const std::string &type_name);

    static bool ValueEqual(Oid type_oid, Datum a, Datum b);
};
