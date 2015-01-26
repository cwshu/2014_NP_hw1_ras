#ifndef __NUMBER_POOL_H__
#define __NUMBER_POOL_H__

#include <vector>
#include <queue>

struct number_pool{
    int min;
    int max;
    std::priority_queue<int, std::vector<int>, std::greater<int>> pool;
    std::vector<bool> is_in_pool; // min => 0, max => max-min;

    number_pool(int min, int max);
    int get_min_idle_id();
    int release_id(int id);
};

#endif /* end of include guard: __NUMBER_POOL_H__ */
