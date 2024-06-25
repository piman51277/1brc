#include "worker.h"

void worker(uint8_t *file, uint64_t size, uint64_t start, uint64_t end, int threadID, StationMap **output)
{
  // seek to find correct bounds.
  // rules are first \n after start, and first \n after end
  uint64_t startPtr = start;
  if (threadID != 0) // first thread already starts aligned
  {
    while (file[startPtr] != '\n')
    {
      startPtr++;
    }
    startPtr++;
  }

  uint64_t endPtr = end;
  while (file[endPtr] != '\n')
  {
    endPtr++;
  }

  uint64_t searchSize = endPtr - startPtr;

  // allocated chunks of memory for cache
  uint8_t *nameCache = new uint8_t[101]; // names are max 100 bytes + null terminator
  uint8_t *valueCache = new uint8_t[5];  // maximum size is of format -?0.0

  // hashmap to store station data
  StationMap *stationMap = output[threadID];
  stationMap->reserve(2000); // reserve space for 10k entries

  // start parsing
  for (uint64_t ptr = startPtr; ptr < endPtr;)
  {
    // read name
    uint64_t nameStart = ptr;
    uint8_t nameLen = 0;
    while (file[ptr] != ';')
    {
      nameCache[nameLen++] = file[ptr++];
    }
    nameCache[nameLen] = 0; // null terminate

    ptr++; // skip the value delim

    // read value
    uint8_t valueLen = 0;
    while (file[ptr] != '\n')
    {
      valueCache[valueLen++] = file[ptr++];
    }

    // skip the entry delim
    ptr++;

    // this one doesn't need to be null terminated, we have a custom func
    int16_t value = parseVal(valueCache);

    // attempt to get the relevant station entry
    auto it = stationMap->find(nameCache);

    // does an entry exist?
    if (it == stationMap->end())
    {
      // create a new entry
      StationData data;
      // copy name
      uint8_t *namecpy = new uint8_t[nameLen + 1];
      std::memcpy(namecpy, nameCache, nameLen + 1);
      data.name = namecpy;
      data.min = value;
      data.max = value;
      data.sum = value;
      data.count = 1;

      stationMap->emplace(namecpy, data);
    }
    else
    {
      // update existing entry
      StationData &data = it->second;
      data.min = std::min(data.min, value);
      data.max = std::max(data.max, value);
      data.sum += value;
      data.count++;
    }
  }
}