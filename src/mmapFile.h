#pragma once
#include <cinttypes>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

struct fileContents
{
  uint8_t *file;
  int64_t size;
};

fileContents *mmapFile(const char *filename);
