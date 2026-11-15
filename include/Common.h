#pragma once
enum class Strategy {
    FIRST_FIT,
    BEST_FIT,
    WORST_FIT
};
enum class Allocate{
    SUCESS,
    NO_SPACE,
    FRAGMENTATION,
    REJECTED
};
