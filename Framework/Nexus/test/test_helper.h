#include <string>

namespace NexusTest {

/**
 * Conditionally remove the named file, it is exists
 */
void removeFile(std::string const &filename);

/**
 * Let's face it, std::string is poorly designed,
 * and this is the constructor that it needed to have.
 * Initialize a string from a c-style formatting string.
 */
std::string strmakef(char const *const fmt, ...);

} // namespace NexusTest
