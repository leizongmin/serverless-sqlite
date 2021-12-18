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

std::ostream &operator<<(std::ostream &os, sls::vfs::DatabaseFile *file) {
  if (file == nullptr) {
    return os << "nullptr";
  } else {
    return os << "*DatabaseFile{name=" << file->name
              << ",iVersion=" << file->base.pMethods->iVersion
              << ", block_size=" << file->block_size << "}";
  }
}

namespace sls::utils {
void dump_env(char **envp) {
  for (auto i = 0; envp[i]; i++) {
    std::cerr << "dump_env: " << envp[i] << std::endl;
  }
}

bool touch_file(const char *path) {
  if (!std::filesystem::exists(path)) {
    auto fp = std::fopen(path, "w+");
    if (fp == nullptr) {
      LOG(ERROR) << "touch_file: " << path << " failed: " << strerror(errno);
      return false;
    }
    std::fclose(fp);
  }
  return true;
}

std::size_t read_from_file(const char *path, void *data, std::int64_t offset,
                           std::size_t n) {
  FILE *fp = fopen(path, "r");
  if (fp == nullptr) {
    LOG(ERROR) << "read_from_file: failed to open file: path=" << path
               << " error=" << strerror(errno);
    return 0;
  }

  std::fseek(fp, offset, SEEK_SET);
  std::size_t m = std::fread(data, 1, n, fp);
  std::fclose(fp);
  if (n != m) {
    LOG(ERROR) << "read_from_file: read bytes unexpected: " << path
               << " expected=" << n << " actual=" << m << std::endl;
  }

  return m;
}

std::size_t write_to_file(const char *path, const void *data,
                          std::int64_t offset, std::size_t n) {
  std::FILE *fp = std::fopen(path, "r+");
  if (fp == nullptr) {
    if (errno == ENOENT) {
      fp = std::fopen(path, "w+");
      if (fp) {
        goto write;
      }
    }
    LOG(ERROR) << "write_to_file: failed to open file: path=" << path
               << " error=" << strerror(errno) << std::endl;
    return 0;
  }

write:
  std::fseek(fp, offset, SEEK_SET);
  std::size_t m = std::fwrite(data, 1, n, fp);
  std::fclose(fp);

  if (n != m) {
    LOG(ERROR) << "write_to_file: write bytes unexpected: " << path
               << " expected=" << n << " actual=" << m << std::endl;
  }

  return m;
}
}  // namespace sls::utils
