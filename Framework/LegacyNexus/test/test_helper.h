#include <string>

namespace LegacyNexusTest {

/**
 * Let's face it, std::string is poorly designed,
 * and this is the constructor that it needed to have.
 * Initialize a string from a c-style formatting string.
 */
std::string strmakef(char const *const fmt, ...);

} // namespace LegacyNexusTest
