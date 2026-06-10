#include "csv.hpp"
#include <print>
#include <iostream>

int main() {
    csv::CSV c;
    c.load("./example.csv", true);
    std::println("{}", c);
    std::cout << c << std::endl;
}