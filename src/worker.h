#pragma once
#include <cinttypes>
#include <unordered_map>
#include <cstring>
#include <functional>
#include <algorithm>
#include <string>
#include "parseVal.h"

constexpr std::size_t FNV_OFFSET_BASIS = 0x01000193;
constexpr std::size_t FNV_PRIME = 0x811c9dc5;

struct StationData
{
  uint8_t *name;
  int16_t min;
  int16_t max;
  int64_t sum;
  uint32_t count;
};

// custom hashmap structs
struct CStrHash
{
  // FNV-1a hash for 32 bit
  std::size_t operator()(const uint8_t *str) const
  {
    std::size_t hash = FNV_OFFSET_BASIS;
    uint8_t c;

    while ((c = *str++))
    {
      hash ^= c;
      hash *= FNV_PRIME;
    }

    return hash;
  }
};

struct CStrEqual
{
  bool operator()(const uint8_t *str1, const uint8_t *str2) const
  {
    return std::strcmp(reinterpret_cast<const char *>(str1), reinterpret_cast<const char *>(str2)) == 0;
  }
};

typedef std::unordered_map<const uint8_t *, StationData, CStrHash, CStrEqual> StationMap;

void worker(uint8_t *file, uint64_t size, uint64_t start, uint64_t end, int threadID, StationMap **output);