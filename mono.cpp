#include <algorithm>
#include <cinttypes>
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <thread>
#include <unordered_map>
#include <vector>

constexpr int16_t p10 = 10;        // 10 value used to automatically promote
constexpr int16_t p100 = 100;      // 100 value used to automatically promote
constexpr int16_t d0 = '0';        // ASCII value, 0
constexpr int16_t d02d = d0 * 11;  // difference for form 0.0
constexpr int16_t d03d = d0 * 111; // difference for form 00.0

constexpr std::size_t FNV_OFFSET_BASIS = 0x01000193;
constexpr std::size_t FNV_PRIME = 0x811c9dc5;

constexpr const char *filename = "measurements.txt";

// parses numbers in format -?0.0
int16_t parseVal(uint8_t *ch)
{
  if (ch[0] == '-')
  {
    if (ch[2] == '.')
    {
      return (ch[1] * p10 + ch[3] - d02d) * -1;
    }
    else
    {
      return (ch[1] * p100 + ch[2] * p10 + ch[4] - d03d) * -1;
    }
  }
  else
  {
    if (ch[1] == '.')
    {
      return ch[0] * p10 + ch[2] - d02d;
    }
    else
    {
      return ch[0] * p100 + ch[1] * p10 + ch[3] - d03d;
    }
  }
}

struct fileContents
{
  uint8_t *file;
  int64_t size;
};

fileContents *mmapFile(const char *filename)
{
  int fd = open(filename, O_RDONLY);
  struct stat fileStat;
  int status = fstat(fd, &fileStat);
  int64_t size = fileStat.st_size;
  fileContents *contents = new fileContents;
  contents->file = static_cast<uint8_t *>(mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0));
  contents->size = size;
  return contents;
}

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

// worker function
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

// below functions mainly exist for profiling purposes
StationMap *mergeMaps(StationMap **output, int numThreads)
{
  StationMap *finalMap = new StationMap();
  for (int i = 0; i < numThreads; i++)
  {
    auto it = output[i]->begin();
    while (it != output[i]->end())
    {
      auto key = it->first;
      auto value = it->second;

      // if the key is not in the final map, insert it
      if (finalMap->find(key) == finalMap->end())
      {
        finalMap->insert({key, value});
      }
      else
      {
        // if the key is in the final map, update the values
        StationData *finalData = &(*finalMap)[key];
        finalData->min = std::min(finalData->min, value.min);
        finalData->max = std::max(finalData->max, value.max);
        finalData->sum += value.sum;
        finalData->count += value.count;
      }

      it++;
    }
  }

  return finalMap;
}

void printResult(StationMap *finalMap)
{
  const char **keys = new const char *[finalMap->size()];
  int i = 0;
  for (auto it = finalMap->begin(); it != finalMap->end(); it++)
  {
    keys[i++] = reinterpret_cast<const char *>(it->first);
  }

  // sort the keys
  std::sort(keys, keys + finalMap->size(), [](const char *a, const char *b)
            { return std::strcmp(a, b) < 0; });

  // print the keys
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(1);
  for (int i = 0; i < finalMap->size(); i++)
  {
    auto key = keys[i];
    auto value = (*finalMap)[reinterpret_cast<const uint8_t *>(key)];
    oss << key << ";" << value.min * 0.1 << ";" << (double)value.sum / value.count * 0.1 << ";" << value.max * 0.1 << "\n";
  }
  std::cout << oss.str();
}

// main
int main()
{
  fileContents *fileContents = mmapFile(filename);
  uint8_t *file = fileContents->file;
  uint64_t size = fileContents->size;

  int numThreads = std::thread::hardware_concurrency();
  uint64_t threadBytes = size / numThreads;

  // create an array of references to StationMaps
  std::vector<StationMap *> output;
  output.resize(numThreads);
  std::thread threads[numThreads];

  for (int i = 0; i < numThreads; i++)
  {
    output[i] = new StationMap();

    // we do -2 so the seeking algorithm actually works
    uint64_t endPtr = i < numThreads - 1 ? (i + 1) * threadBytes : size - 2;

    // only spawn one thread for debugging
    threads[i] = std::thread(worker, file, size, i * threadBytes, endPtr, i, output.data());
  }

  // wait for all threads to finish
  for (int i = 0; i < numThreads; i++)
  {
    threads[i].join();
  }

  // merge all the maps
  StationMap *finalMap = mergeMaps(output.data(), numThreads);

  // print the final map
  printResult(finalMap);

  exit(0);
}