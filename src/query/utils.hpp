#include <cstddef>
#include <cstdint>

namespace sls::utils {
void dump_env(char **envp);
bool touch_file(const char *path);
std::size_t read_from_file(const char *path, void *data, std::int64_t offset,
                           std::size_t n);
std::size_t write_to_file(const char *path, const void *data,
                          std::int64_t offset, std::size_t n);
}  // namespace sls::utils
