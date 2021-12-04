#include <string>

#include "sqlite3.h"

namespace sls::vfs {
struct slsFile {
  sqlite3_file base;
  std::string name;
  int blockSize;
};
void registerVfs();
}  // namespace sls::vfs