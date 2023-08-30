#include "mem_map.hpp"
#include <set>
#include <random>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
namespace pt = boost::property_tree;
#define BIT_SET(a,b) ((a) |= (1ULL<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1ULL<<(b)))
#define BIT_CHECK(a,b) (!!((a) & (1ULL<<(b)))) 
#define MASK_CLEAR(a,b) ((a) &= ~(b))
static size_t mylog2(u_int64_t val){
    if (val == 0) return UINT_MAX;
    if (val == 1) return 0;
    size_t ret = 0;
    while (val > 1) {
        val >>= 1;
        ret++;
    }
    return ret;
}
static size_t pow2(const size_t val){
    return 1ULL<<val;
}
script::script(const std::string& filename){
    pt::ptree root;
    //Reading Json file into ptree
    try{
        pt::read_json(filename, root);
    }catch(pt::json_parser::json_parser_error& e1){
        throw std::runtime_error(e1.what());
    }
    try{
        //Copying bank_addr_bits from ptree to vector
        for(const auto& bit : root.get_child("cache_descr.bnk_addr_bits"))
            this->cache.bnk_addr.push_back(bit.second.get_value<size_t>());
        //Copying cache parameters from ptree to structure
        this->cache.line_sz = root.get<size_t>("cache_descr.line_size");
        this->cache.assoc = root.get<size_t>("cache_descr.associativity");
        this->cache.cache_sz = root.get<size_t>("cache_descr.cache_size");
        //Copying addr_ranges from ptree to vector
        for(const auto& addr : root.get_child("addr_ranges"))
            this->addr_ranges.push_back(std::make_pair(std::stoull(addr.second.get<std::string>("bgn"), nullptr, 16), std::stoull(addr.second.get<std::string>("end"), nullptr, 16)));
        //Copying set_range from ptree to vector
        this->set_rng.first = root.get<size_t>("set_range.min");
        this->set_rng.second = root.get<size_t>("set_range.max");
        //Copying bnk_range from ptree to vector
        this->bnk_rng.first = root.get<size_t>("bnk_range.min");
        this->bnk_rng.second = root.get<size_t>("bnk_range.max");
        //Copying lines_assoc_coeff from ptree to vector
        this->lines_coeff = root.get<double>("lines_assoc_coeff");
    }catch(pt::ptree_bad_path& e2){
        throw std::runtime_error(e2.what());
    }
}
std::set<u_int64_t> mapgen(size_t seed, const script& task){
    std::mt19937_64 gen64;
    gen64.seed(seed);
    size_t bnk, set, addr;
    u_int64_t buf;
    std::set<u_int64_t> res;
    size_t bnk_am = pow2(task.cache.bnk_addr.size());
    size_t sets_am = task.cache.cache_sz / task.cache.line_sz / bnk_am / task.cache.assoc;
    size_t sets = gen64()%(task.set_rng.second - task.set_rng.first + 1) + task.set_rng.first;
    size_t bnks = gen64()%(task.bnk_rng.second - task.bnk_rng.first + 1) + task.bnk_rng.first;
    size_t lines_am = (size_t)(task.cache.assoc * task.lines_coeff);
    std::set<size_t> set_excl;
    std::set<size_t> bnk_excl;
    std::vector<size_t> set_addr;
    for(size_t i = mylog2(task.cache.line_sz) + 1; set_addr.size() < mylog2(sets_am); i++){
        if(find(task.cache.bnk_addr.begin(), task.cache.bnk_addr.end(), i) == task.cache.bnk_addr.end())
            set_addr.push_back(i);
    }
    for(size_t i = 0; i < bnks; i++){
        bnk = gen64()%bnk_am;
        while(bnk_excl.find(bnk) != bnk_excl.end()) bnk = (bnk + 1)%bnk_am;
        bnk_excl.insert(bnk);
        for(size_t j = 0; j < sets; j++){
            set = gen64()%sets_am;
            while(set_excl.find(set) != set_excl.end()) set = (set + 1)%sets_am;
            set_excl.insert(set);
            for(size_t k = 0; k < lines_am; k++){
                do{
                    addr = gen64()%task.addr_ranges.size();
                    buf = gen64()%(task.addr_ranges[addr].second - task.addr_ranges[addr].first + 1) + task.addr_ranges[addr].first;
                    //Setting offset to 0
                    MASK_CLEAR(buf, task.cache.line_sz - 1);
                    //Setting bank number
                    for(size_t m = 0; m < task.cache.bnk_addr.size(); m++){
                        if(BIT_CHECK(bnk, m))
                            BIT_SET(buf, task.cache.bnk_addr[m] - 1);
                        else
                            BIT_CLEAR(buf, task.cache.bnk_addr[m] - 1);
                    }
                    //Setting set number
                    for(size_t m = 0; m < set_addr.size(); m++){
                        if(BIT_CHECK(set, m))
                            BIT_SET(buf, set_addr[m] - 1);
                        else
                            BIT_CLEAR(buf, set_addr[m] - 1);
                    }
                }while(buf < task.addr_ranges[addr].first || buf > task.addr_ranges[addr].second || find(res.begin(), res.end(), buf) != res.end());
                res.insert(buf);
            }
        }
    }
    return res;
}
void to_json(const std::string& file, const std::set<u_int64_t>& buf){
    pt::ptree root;
    pt::ptree addr_ranges;
    std::stringstream hex;
    for(const auto& addr : buf){
        pt::ptree range;
        hex << std::hex << addr;
        range.put("bgn", "0x" + hex.str());
        hex.str(std::string());
        hex << std::hex << addr + 63;
        range.put("end", "0x" + hex.str());
        hex.str(std::string());
        addr_ranges.push_back(std::make_pair("", range));
    }
    root.add_child("addr_ranges", addr_ranges);
    write_json(file, root);
}