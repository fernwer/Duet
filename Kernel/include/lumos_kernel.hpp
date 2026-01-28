#pragma onece

#include <string>
#include <memory>
#include "exec/executor.hpp"


extern "C" {
#include "postgres.h"
#include "utils/elog.h"
}

class LumosKernel {
public:
    LumosKernel();
    ~LumosKernel();

    void Dispatch(const char* data, size_t len);

    std::string DebugAnalyze(const char* data, size_t len);

private:
    std::unique_ptr<Executor> executor_;
};
