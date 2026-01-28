#pragma once

#include "common.hpp"

class SQLParser {
public:
    static bool Analyze(int req_id, const std::string& sql, ParsedQuery& out_result);

private:
};
