#include <cstdint>
#include <cstddef>

namespace sls::utils {
void dumpEnv(char **envp);
bool touchFile(const char *path);
std::size_t readFromFile(const char *path, void *data, std::int64_t offset,
                         std::size_t n);
std::size_t writeToFile(const char *path, const void *data, std::int64_t offset,
                        std::size_t n);
}  // namespace sls::utils
