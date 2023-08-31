#include "mem_map.hpp"
#include <iostream>
#include <string>
#include <boost/program_options.hpp>
namespace po = boost::program_options;
int main(int argc, char *argv[]){
    po::options_description desc("Options");
    po::positional_options_description pos_desc;
    size_t seed;
    std::string file;
    std::string result;
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
    try{
        script task(file);
        to_json(result, mapgen(seed, task), task);
    }
    catch(std::runtime_error& e){
        std::cerr<<e.what()<<std::endl;
        return 1;
    }
    return 0;
}