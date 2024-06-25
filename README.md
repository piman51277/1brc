# 1brc - 1 Billion Row Challenge
[Challenge website](https://1brc.dev/)

Data generated from [this repo](https://github.com/dannyvankooten/1brc#submitting)

## Results:
Time (local): **~3.0 seconds**

### Env
- COMPILER: g++ 12.2.0 (debian) flags `-O3 -march=native --std=c++23`
- CPU: AMD Ryzen 9 3900x
- RAM: 32GB DDR4
- OS: Debian 12 (Bookworm)
- Kernel: 6.1.0-21-amd64 

## Running
Run `./cr.sh` to **C**ompile and **R**un.

Expects `measurements.txt` to be present in project root. (Recommended to place file on your fastest drive and symlink it here)

## Profiling
Requires `hotpost` (`sudo apt install hotspot`).

Run `./prof.sh` to compile, profile, and launch hotspot.

## Notes
- Will only work on Linux and some other POSIX systems use due to the use of POSIX API calls.
- `src/` contains the code as it was written. `mono.cpp` is an alternative version that was packed into a single file.