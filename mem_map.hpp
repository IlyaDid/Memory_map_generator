#include "cache.hpp"
#include <set>
class script{
public:
    Cache cache;
    std::vector<std::pair<u_int64_t, u_int64_t>> addr_ranges;
    std::pair<size_t, size_t> set_rng;
    std::pair<size_t, size_t> bnk_rng;
    double lines_coeff;

    script(const std::string& filename);
};
std::set<u_int64_t> mapgen(size_t seed, const script& task);
void to_json(const std::string& file, const std::set<u_int64_t>& buf);