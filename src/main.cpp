
#include <iostream>
#include <thread>
#include <functional>
#include "mmapFile.h"
#include "worker.h"
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>

constexpr const char *filename = "/tmp/1brc/measurements.txt";

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