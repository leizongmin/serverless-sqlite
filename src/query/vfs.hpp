#include <string>

#include "sqlite3.h"

namespace sls::vfs {
struct DatabaseFile {
  sqlite3_file base;
  std::string name;
  int block_size;
};
void register_vfs();
}  // namespace sls::vfs