#include "Cache.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <time.h>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#define BIT_SET(a,b) ((a) |= (1ULL<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1ULL<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (1ULL<<(b)))
#define BIT_CHECK(a,b) (!!((a) & (1ULL<<(b)))) 
#define MASK_CLEAR(a,b) ((a) &= ~(b))
namespace pt = boost::property_tree;
struct script{
    Cache cache;
    std::vector<std::pair<u_int64_t, u_int64_t>> addr_ranges;
    std::pair<size_t, size_t> set_rng;
    std::pair<size_t, size_t> bnk_rng;
    double lines_coeff;
};
size_t mylog2(u_int64_t val) {
    if (val == 0) return UINT_MAX;
    if (val == 1) return 0;
    size_t ret = 0;
    while (val > 1) {
        val >>= 1;
        ret++;
    }
    return ret;
}
size_t pow2(size_t val){
    return 1ULL<<val;
}
script GetTaskInfo(const std::string& filename){
    pt::ptree root;
    script task;
    //Reading Json file into ptree
    try{
        pt::read_json(filename, root);
    }catch(pt::json_parser::json_parser_error& e1){
        throw std::runtime_error(e1.what());
    }
    try{
        //Copying bank_addr_bits from ptree to vector
        for(const auto& bit : root.get_child("cache_descr.bnk_addr_bits"))
            task.cache.bnk_addr.push_back(bit.second.get_value<size_t>());
        //Copying cache parameters from ptree to structure
        task.cache.line_sz = root.get<size_t>("cache_descr.line_size");
        task.cache.assoc = root.get<size_t>("cache_descr.associativity");
        task.cache.cache_sz = root.get<size_t>("cache_descr.cache_size");
        //Copying addr_ranges from ptree to vector
        for(const auto& addr : root.get_child("addr_ranges"))
            task.addr_ranges.push_back(std::make_pair(std::stoull(addr.second.get<std::string>("bgn"), nullptr, 16), std::stoull(addr.second.get<std::string>("end"), nullptr, 16)));
        //Copying set_range from ptree to vector
        task.set_rng.first = root.get<size_t>("set_range.min");
        task.set_rng.second = root.get<size_t>("set_range.max");
        //Copying bnk_range from ptree to vector
        task.bnk_rng.first = root.get<size_t>("bnk_range.min");
        task.bnk_rng.second = root.get<size_t>("bnk_range.max");
        //Copying lines_assoc_coeff from ptree to vector
        task.lines_coeff = root.get<double>("lines_assoc_coeff");

        // std::cout << task.cache.line_sz << " " << task.cache.assoc << " " << task.cache.cache_sz << std::endl;
        // for(const auto& it : task.cache.bnk_addr)
        //     std::cout << it << " ";
        // std::cout << std::endl;
        // for(const auto& it : task.addr_ranges)
        //     std::cout << it.first << "," << it.second << std::endl;
        // std::cout << task.set_rng.first << "," << task.set_rng.second << std::endl;
        // std::cout << task.bnk_rng.first << "," << task.bnk_rng.second << std::endl;
        // std:: cout << task.lines_coeff << std::endl;

    }catch(pt::ptree_bad_path& e2){
        throw std::runtime_error(e2.what());
    }
    return task;
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
    for(size_t i = (size_t)mylog2(task.cache.cache_sz); set_addr.size() < (size_t)mylog2(sets_am); i++){
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
                    buf = MASK_CLEAR(buf, task.cache.line_sz - 1);
                    //Setting bank number
                    for(size_t m = 0; m < task.cache.bnk_addr.size(); m++){
                        if(BIT_CHECK(bnk, m))
                            buf = BIT_SET(buf, task.cache.bnk_addr[m]);
                        else
                            buf = BIT_CLEAR(buf, task.cache.bnk_addr[m]);
                    }
                    //Setting set number
                    for(size_t m = 0; m < set_addr.size(); m++){
                        if(BIT_CHECK(set, m))
                            buf = BIT_SET(buf, set_addr[m]);
                        else
                            buf = BIT_CLEAR(buf, set_addr[m]);
                    }
                }while(buf < task.addr_ranges[addr].first || buf > task.addr_ranges[addr].second || find(res.begin(), res.end(), buf) != res.end());
                res.insert(buf);
            }
        }
    }
    return res;
    // bool check;
    // size_t count = 0;
    // size_t am = 0;
    // size_t evict = 0;
    // for(const auto& it : res){
    //     if(it%64 != 0){std::cout << "ERROR! " << it <<std::endl;return res;}
    //     if(count == 6){am = 0; count = 0;}
    //     check = false;
    //     for(const auto& address : task.addr_ranges)
    //         if(it >= address.first && it <= address.second)
    //             check = true;
    //     if(!check){std::cout << "ERROR! " << it <<std::endl;return res;}
    //     check = true;
    //     for(const auto& bit : set_addr){
    //         if(BIT_CHECK(it, bit) ^ BIT_CHECK(it+1, bit)){
    //             check = false;
    //             break;
    //         }
    //     }
    //     if(check) am++;
    //     count++;
    //     if(am>4) evict++;
    // }
    // std::cout << bnks << " " << sets << " " << evict << std::endl;
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
namespace po = boost::program_options;
int main(int argc, char *argv[]){
    po::options_description desc("Options");
    po::positional_options_description pos_desc;
    size_t seed;
    std::string file;
    std::string result;
    script task;
    po::variables_map vm;
    desc.add_options()
        ("help,h", "Show help")
        ("seed,s", po::value<size_t>(&seed)->required(), "Seed for pseudorandom generation")
        ("file,f", po::value<std::string>(&file)->required(), "JSON file with description of cache and memory parameters")
        ("result,r", po::value<std::string>(&result)->required(), "JSON file where result will be saved")
    ;
    pos_desc.add("file", 1);
    po::parsed_options parsed = po::command_line_parser(argc, argv).options(desc).positional(pos_desc).run();
    po::store(parsed, vm);
    if(vm.count("help")){std::cerr<<desc<<std::endl;return 0;}
    po::notify(vm);

    // size_t size = 0;
    // size_t sz = 0;
    // std::mt19937_64 gen64;

    try{
        // gen64.seed(seed);
        task = GetTaskInfo(file);
        to_json(result, mapgen(seed, task));
        // for(const auto& addr : buf)
        //     std::cout << "0x" << std::hex << addr << std::endl;
        // for(const auto& it : task.addr_ranges)
        //     size += it.second - it.first;
        // time_t start = time(nullptr);
        // for(size_t i = 0; i < 100000; i++){
        //     seed = gen64();
        //     buf = mapgen(seed, task);
        //     sz += buf.size();
        // }
        // std::cout << time(nullptr) - start << std::endl;
        //std::cout << sz*64 << " " << size << " " << (double)((double)sz*64/(double)size)*100 << "%" << std::endl;
    }
    catch(std::runtime_error& e){
        std::cerr<<e.what()<<std::endl;
        return 1;
    }
    return 0;
}