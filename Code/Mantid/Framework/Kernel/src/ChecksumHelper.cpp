#include "MantidKernel/ChecksumHelper.h"

#include <boost/regex.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_array.hpp>

#include <Poco/MD5Engine.h>
#include <Poco/SHA1Engine.h>
#include <Poco/DigestStream.h>

#include <fstream>
#include <sstream>
#include <string>

namespace Mantid {
namespace Kernel {

namespace ChecksumHelper {
namespace // anonymous
    {
/**
 * Load contents of file into a string. The line endings are preserved
 * @param filepath Full path to the file to be opened
 * @param unixEOL If true convert all lineendings to Unix-style \n
 */
std::string loadFile(const std::string &filepath, const bool unixEOL = false) {
  std::ifstream filein(filepath.c_str(), std::ios::in | std::ios::binary);
  if (!filein)
    return "";

  std::string contents;
  filein.seekg(0, std::ios::end);
  contents.resize(filein.tellg());
  filein.seekg(0, std::ios::beg);
  filein.read(&contents[0], contents.size());
  filein.close();

  if (unixEOL) {
    static boost::regex eol(
        "\\R"); // \R is Perl syntax for matching any EOL sequence
    contents = boost::regex_replace(contents, eol, "\n"); // converts all to LF
  }
  return contents;
}

/**
 * Create sha1 out of data and an optional header
 * @param data Contents as a string
 * @param header An optional string to prepend to the data
 */
std::string createSHA1(const std::string &data,
                       const std::string &header = "") {
  using Poco::DigestEngine;
  using Poco::SHA1Engine;
  using Poco::DigestOutputStream;

  SHA1Engine sha1;
  DigestOutputStream outstr(sha1);
  outstr << header << data;
  outstr.flush(); // to pass everything to the digest engine
  return DigestEngine::digestToHex(sha1.digest());
}

/**
 * Create sha1 out of data and an optional header
 * @param data Contents as a string
 * @param header An optional string to prepend to the data
 */
std::string createMD5(const std::string &data, const std::string &header = "") {
  using Poco::DigestEngine;
  using Poco::MD5Engine;
  using Poco::DigestOutputStream;

  MD5Engine sha1;
  DigestOutputStream outstr(sha1);
  outstr << header << data;
  outstr.flush(); // to pass everything to the digest engine
  return DigestEngine::digestToHex(sha1.digest());
}
}

/** Creates a md5 checksum from a string
* @param input The string to checksum
* @returns a checksum string
**/
std::string md5FromString(const std::string &input) {
  return ChecksumHelper::createMD5(input);
}

/** Creates a SHA-1 checksum from a string
* @param input The string to checksum
* @returns a checksum string
**/
std::string sha1FromString(const std::string &input) {
  return ChecksumHelper::createSHA1(input);
}

/** Creates a SHA-1 checksum from a file
* @param filepath The path to the file
* @returns a checksum string
**/
std::string sha1FromFile(const std::string &filepath) {
  if (filepath.empty())
    return "";
  return ChecksumHelper::createSHA1(loadFile(filepath));
}

/** Creates a git checksum from a file (these match the git hash-object
*command).
* This works by reading in the file, converting all line endings into linux
*style endings,
* then the following is prepended to the file contents "blob
*<content_length>\0",
* the result is then ran through a SHA-1 checksum.
* @param filepath The path to the file
* @returns a checksum string
**/
std::string gitSha1FromFile(const std::string &filepath) {
  if (filepath.empty())
    return "";
  const bool unixEOL(true);
  std::string contents = loadFile(filepath, unixEOL);
  std::stringstream header;
  header << "blob " << contents.size() << '\0';
  return ChecksumHelper::createSHA1(contents, header.str());
}

} // namespace ChecksumHelper
} // namespace Kernel
} // namespace Mantid
