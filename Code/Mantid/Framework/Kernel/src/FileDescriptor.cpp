#include "MantidKernel/FileDescriptor.h"
#include "MantidKernel/Exception.h"

#include <Poco/File.h>
#include <Poco/Path.h>

namespace Mantid {
namespace Kernel {
//----------------------------------------------------------------------------------------------
// Public static methods
//----------------------------------------------------------------------------------------------
/**
 * Check if the given file appears to be an ASCII file. The check searches the
 * first nbytes of the file and returns false if a non-ascii character is found.
 * If the file is shorter than nbytes then it checks until the end of the
 * stream.
 * @param filename A string pointing to an existing file
 * @param nbytes The number of bytes of the file to check (Default=256)
 * @returns True if the file is considered ASCII, false otherwise
 * @throws std::invalid_argument if the file cannot be opened
 * @throws std::runtime_error if an error is occurred while reading the stream
 */
bool FileDescriptor::isAscii(const std::string &filename, const size_t nbytes) {
  std::ifstream data(filename.c_str(), std::ios::in | std::ios::binary);
  if (!data) {
    throw std::invalid_argument(
        "FileDescriptor::isAscii() - Unable to open file '" + filename + "'");
  }
  return FileDescriptor::isAscii(data, nbytes);
}

/**
 * Check if the given stream appears to point to an ASCII data. The check
 * searches the
 * next nbytes of the stream and returns false if a non-ascii character is
 * found.
 * If the stream is shorter than nbytes or a reading error occurs then it simply
 * returns
 * the result up to that point
 * The stream is reset to the position is was at when entering the function
 * @param data An input stream opened in binary mode
 * @param nbytes The number of bytes of the file to check (Default=256)
 * @returns True if the stream is considered ASCII, false otherwise
 */
bool FileDescriptor::isAscii(std::istream &data, const size_t nbytes) {
  const std::streampos startPos = data.tellg();
  char byte('\0');
  size_t counter(0);
  bool result(true);
  while (counter < nbytes) {
    data >> byte;
    if (!data) // reading error
    {
      data.clear();
      break;
    }
    unsigned long ch = static_cast<unsigned long>(byte);
    if (!(ch <= 0x7F)) // non-ascii
    {
      result = false;
      break;
    }
    ++counter;
  }

  data.seekg(startPos);
  return result;
}

/**
* Check if a file is a text file
* @param file :: The file pointer
* @param nbytes The number of bytes of the file to check (Default=256)
* @returns true if the file an ascii text file, false otherwise
*/
bool FileDescriptor::isAscii(FILE *file, const size_t nbytes) {
  // read the data and reset the seek index back to the beginning
  char *data = new char[nbytes];
  char *pend = &data[fread(data, 1, nbytes, file)];
  fseek(file, 0, SEEK_SET);

  // Call it a binary file if we find a non-ascii character in the
  // first nbytes bytes of the file.
  bool result = true;
  for (char *p = data; p < pend; ++p) {
    unsigned long ch = (unsigned long)*p;
    if (!(ch <= 0x7F)) {
      result = false;
      break;
    }
  }
  delete[] data;

  return result;
}

//----------------------------------------------------------------------------------------------
// Public methods
//----------------------------------------------------------------------------------------------

/**
 * @param filename A string containing a filename. The file must exist
 * @throws std::invalid_argument if the filename is empty or the file does not
 * exist
 */
FileDescriptor::FileDescriptor(const std::string &filename)
    : m_filename(), m_extension(), m_file(), m_ascii(false) {
  if (filename.empty()) {
    throw std::invalid_argument("FileDescriptor() - Empty filename '" +
                                filename + "'");
  }
  if (!Poco::File(filename).exists()) {
    throw std::invalid_argument("FileDescriptor() - File '" + filename +
                                "' does not exist");
  }
  initialize(filename);
}

/**
 * Closes the file handle
 */
FileDescriptor::~FileDescriptor() { m_file.close(); }

/**
 * Moves the stream pointer back to the start of the file, without
 * reopening the file. Note that this will affect the stream that
 * has been accessed using the stream() method
 */
void FileDescriptor::resetStreamToStart() {
  m_file.close(); // reset all flags
  // If the stream was closed, reopen it
  if (!m_file.is_open()) {
    m_file.open(m_filename.c_str(), std::ios::in | std::ios::binary);
  } else {
    m_file.seekg(0);
  }
}

//----------------------------------------------------------------------------------------------
// Private methods
//----------------------------------------------------------------------------------------------

/**
 * Set the description fields and opens the file
 * @param filename A string pointing to an existing file
 */
void FileDescriptor::initialize(const std::string &filename) {
  m_filename = filename;
  m_extension = "." + Poco::Path(filename).getExtension();

  m_file.open(m_filename.c_str(), std::ios::in | std::ios::binary);
  if (!m_file)
    throw std::runtime_error("FileDescriptor::initialize - Cannot open file '" +
                             filename + "' for reading");

  m_ascii = FileDescriptor::isAscii(m_file);
}

} // namespace Kernel
} // namespace Mantid
