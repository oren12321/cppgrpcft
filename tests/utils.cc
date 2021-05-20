#include "utils.h"

#include <fstream>
#include <random>
#include <algorithm>
#include <vector>
#include <iterator>
#include <functional>
#include <string>

std::vector<char> generateRandomBuffer(int size) {
    std::random_device r;
    std::seed_seq seed{};
    std::mt19937 eng(seed);

    std::uniform_int_distribution<int> dist;
    std::vector<char> v(size);

    std::generate(v.begin(), v.end(), std::bind(dist, eng));
    return v;
}

template <typename InputIter>
bool rangeEqual(InputIter begin1, InputIter end1, InputIter begin2, InputIter end2) {
    while (begin1 != end1 && begin2 != end2) {
        if (*begin1 != *begin2) return false;
        ++begin1;
        ++begin2;
    }
    return (begin1 == end1) && (begin2 == end2);
}

bool filesEqual(const std::string& path1, const std::string& path2) {
    std::ifstream ifs1(path1);
    std::ifstream ifs2(path2);

    std::istreambuf_iterator<char> begin1(ifs1);
    std::istreambuf_iterator<char> begin2(ifs2);

    std::istreambuf_iterator<char> end;

    return rangeEqual(begin1, end, begin2, end);
}
