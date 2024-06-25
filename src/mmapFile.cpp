#include "mmapFile.h"

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