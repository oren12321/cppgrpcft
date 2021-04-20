#ifndef __ARGSPARSER_H__
#define __ARGSPARSER_H__

#include <string>
#include <unordered_map>

std::unordered_map<std::string, std::string> parseArgs(int argc, char** argv) {
    std::unordered_map<std::string, std::string> mapped_args;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        size_t start_pos = arg.find("=");
        if (start_pos != std::string::npos && start_pos + 1 < arg.size()) {
            std::string arg_name = arg.substr(0, start_pos);
            std::string arg_val = arg.substr(start_pos + 1);
            mapped_args[arg_name] = arg_val;
        }
    }
    return mapped_args;
}

#endif

