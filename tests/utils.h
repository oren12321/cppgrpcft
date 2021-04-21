#ifndef __UTILS_H__
#define __UTILS_H__

#include <vector>
#include <string>

std::vector<char> generateRandomBuffer(int size);

template <typename InputIter>
bool rangeEqual(InputIter begin1, InputIter end1, InputIter begin2, InputIter end2);

bool filesEqual(const std::string& path1, const std::string& path2);

#endif

