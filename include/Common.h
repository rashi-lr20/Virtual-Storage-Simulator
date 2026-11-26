#ifndef COMMON_H
#define COMMON_H
// Shared types used by every module. Keep this file small and stable.

enum class Strategy { FIRST_FIT, BEST_FIT, WORST_FIT };

enum class AllocResult {
    SUCCESS,
    INVALID_REQUEST,   // size <= 0 or file id out of range
    DUPLICATE_ID,      // file id already allocated
    NO_SPACE,          // total free space is less than the request
    FRAGMENTATION      // total free >= request, but no single segment is big enough
};

inline const char* strategyName(Strategy s) {
    switch (s) {
        case Strategy::FIRST_FIT: return "First Fit";
        case Strategy::BEST_FIT:  return "Best Fit";
        default:                  return "Worst Fit";
    }
}

inline const char* resultName(AllocResult r) {
    switch (r) {
        case AllocResult::SUCCESS:         return "SUCCESS";
        case AllocResult::INVALID_REQUEST: return "REJECTED (invalid request)";
        case AllocResult::DUPLICATE_ID:    return "REJECTED (file id already exists)";
        case AllocResult::NO_SPACE:        return "FAILED (not enough total free space)";
        default:                           return "FAILED (fragmentation: enough total space, no contiguous segment)";
    }
}

#endif
