g++ src/*.cpp -O3 -g -march=native --std=c++23 -o a.out
perf record --call-graph dwarf -F 500 ./a.out
hotspot perf.data