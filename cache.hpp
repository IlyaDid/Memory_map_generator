#include <iostream>
#include <vector>
struct Cache{
size_t line_sz;
size_t assoc;
size_t cache_sz;
std::vector<size_t> bnk_addr;
};