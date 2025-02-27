#include <iosfwd>
#include <string>

// return to system when any test failed
constexpr int TEST_FAILED{1};
// return to system when all tests succeed
constexpr int TEST_SUCCEED{0};
// error out and return failure
#define ON_ERROR(msgstr)                                                                                               \
  {                                                                                                                    \
    std::cerr << msgstr << std::endl;                                                                                  \
    return TEST_FAILED;                                                                                                \
  }

namespace NexusNapiTest {
void print_data(const std::string &prefix, std::ostream &stream, const void *data, const NXnumtype type, const int num);
void write_dmc01(const std::string &filename);
void write_dmc02(const std::string &filename);
/// remove a file if it exists
void removeFile(const std::string &filename);
} // namespace NexusNapiTest
