#include "utils.hpp"

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <iostream>

#include "glog/logging.h"
#include "sqlite3.h"
#include "vfs.hpp"

std::ostream &operator<<(std::ostream &os, sqlite3_vfs *vfs) {
  if (vfs == nullptr) {
    return os << "nullptr";
  } else {
    return os << "*sqlite3_vfs{zName=" << vfs->zName
              << ",iVersion=" << vfs->iVersion << ",szOsFile=" << vfs->szOsFile
              << "}";
  }
}

std::ostream &operator<<(std::ostream &os, sls::vfs::slsFile *file) {
  if (file == nullptr) {
    return os << "nullptr";
  } else {
    return os << "*slsFile{name=" << file->name
              << ",iVersion=" << file->base.pMethods->iVersion
              << ", blockSize=" << file->blockSize << "}";
  }
}

namespace sls::utils {
void dumpEnv(char **envp) {
  for (auto i = 0; envp[i]; i++) {
    std::cerr << "dump_env: " << envp[i] << std::endl;
  }
}

bool touchFile(const char *path) {
  if (!std::filesystem::exists(path)) {
    auto fp = std::fopen(path, "w+");
    if (fp == nullptr) {
      LOG(ERROR) << "touchFile: " << path << " failed: " << strerror(errno);
      return false;
    }
    std::fclose(fp);
  }
  return true;
}

std::size_t readFromFile(const char *path, void *data, std::int64_t offset,
                         std::size_t n) {
  FILE *fp = fopen(path, "r");
  if (fp == nullptr) {
    LOG(ERROR) << "readFromFile: failed to open file: path=" << path
               << " error=" << strerror(errno);
    return 0;
  }

  std::fseek(fp, offset, SEEK_SET);
  std::size_t m = std::fread(data, 1, n, fp);
  std::fclose(fp);
  if (n != m) {
    LOG(ERROR) << "readFromFile: read bytes unexpected: " << path
               << " expected=" << n << " actual=" << m << std::endl;
  }

  return m;
}

std::size_t writeToFile(const char *path, const void *data, std::int64_t offset,
                        std::size_t n) {
  std::FILE *fp = std::fopen(path, "r+");
  if (fp == nullptr) {
    if (errno == ENOENT) {
      fp = std::fopen(path, "w+");
      if (fp) {
        goto write;
      }
    }
    LOG(ERROR) << "writeToFile: failed to open file: path=" << path
               << " error=" << strerror(errno) << std::endl;
    return 0;
  }

write:
  std::fseek(fp, offset, SEEK_SET);
  std::size_t m = std::fwrite(data, 1, n, fp);
  std::fclose(fp);

  if (n != m) {
    LOG(ERROR) << "writeToFile: write bytes unexpected: " << path
               << " expected=" << n << " actual=" << m << std::endl;
  }

  return m;
}
}  // namespace sls::utils
