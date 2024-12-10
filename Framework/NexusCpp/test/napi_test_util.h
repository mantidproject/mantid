#include <iosfwd>

namespace NexusCppTest {
void print_data(const std::string &prefix, std::ostream &stream, const void *data, const int type, const int num);
void write_dmc01(const std::string &filename);
void write_dmc02(const std::string &filename);
/// remove a file if it exists
void removeFile(const std::string &filename);
} // namespace NexusCppTest
