#include <filesystem>
#include <string>

namespace NexusTest {

/**
 * Conditionally remove the named file, it is exists
 */
void removeFile(std::string const &filename);

/**
 * @param filename filename without path
 * @return full path to file. This returns empty string if it doesn't exist.
 */
std::string getFullPath(const std::string &filename);

/**
 * Let's face it, std::string is poorly designed,
 * and this is the constructor that it needed to have.
 * Initialize a string from a c-style formatting string.
 */
std::string strmakef(char const *const fmt, ...);

/**
 * This will check if an HDF file is open.  It will first check if any files are open.
 * If any are, it will looking for the given filename in the list of all opened files.
 * @param filename the filename to check if opened
 * @return true if the file has been closed; otherwise false
 */
bool hdf_file_is_closed(std::string const &filename);

/**
 * Makes a temporary file in a proper temp folder an ensures deletion on creation and exit.
 * NOTE: this is a copy of a framework test helper, put here to simplify NexusTest build tree.
 */
class FileResource {
public:
  FileResource(std::string const &filename, bool debugMode = false);
  void setDebugMode(bool mode);
  std::string fullPath() const;
  ~FileResource();

private:
  bool m_debugMode;
  std::filesystem::path m_full_path;

protected:
  static void *operator new(std::size_t);   // prevent heap allocation of scalar.
  static void *operator new[](std::size_t); // prevent heap allocation of array.
};

} // namespace NexusTest
