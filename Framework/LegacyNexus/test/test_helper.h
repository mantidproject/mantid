#include <string>

namespace LegacyNexusTest {

/**
 * Let's face it, std::string is poorly designed,
 * and this is the constructor that it needed to have.
 * Initialize a string from a c-style formatting string.
 */
std::string strmakef(char const *const fmt, ...);

enum NexusFormat { HDF4, HDF5 };

struct FormatUniqueVars {
  std::string relFilePath;
  std::string rootID;
};

FormatUniqueVars getFormatUniqueVars(const NexusFormat fmt, const std::string &filename);
} // namespace LegacyNexusTest
