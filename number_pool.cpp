#include "number_pool.h"

number_pool::number_pool(int min, int max){
    this->min = min;
    this->max = max;
    for( int i = min; i <= max; ++i ){
        pool.push(i);
    }
    is_in_pool = std::vector<bool>(max-min+1, true);
}

int number_pool::get_min_idle_id(){
    if( pool.empty() ){
        return -1;
    }
    int id = pool.top();
    is_in_pool[id] = false;
    pool.pop();
    return id;
}

int number_pool::release_id(int id){
    if( id < min || id > max ){
        return -1;
    }
    if( is_in_pool[id] == true ){
        return -1;
    }
    pool.push(id);
    is_in_pool[id] = true;
    return 0;
}
