#ifndef COMMON_H
#define COMMON_H

enum class Strategy {
    FIRST_FIT,
    BEST_FIT,
    WORST_FIT,
};

enum class AllocResult{
    SUCCESS,
    INVALID_REQUEST,
    DUPLICATE_ID,
    NO_SPACE,
    FRAGMENTATION,
};

inline const char* strategyName(Strategy s) {
    switch (s) {
        case Strategy::FIRST_FIT: return "First Fit";
        case Strategy::BEST_FIT:  return "Best Fit";
        default:                  return "Worst Fit";
    }
}

inline const char* resultName(AllocResult r) {
    switch(r){
        case AllocResult::SUCCESS:
        return "SUCCESS";
        case AllocResult::INVALID_REQUEST:
        return "REJECTED (invalid request)";
        case AllocResult::DUPLICATE_ID:
        return "REJECTED (file id already exists)";
        case AllocResult::NO_SPACE:
        return "FAILED (not enough total free space)";
        default:
        return "FAILED (fragmentation: enough total space, no contiguous segment)";
    }
}

#endif