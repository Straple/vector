#include <iostream>
#include "vector.hpp"

int main() {
    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    std::cout << vec.size() << std::endl;
    std::cout << vec[0] << ' ' << vec[1] << std::endl;
}