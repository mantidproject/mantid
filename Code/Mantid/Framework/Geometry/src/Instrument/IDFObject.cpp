#include "MantidGeometry/Instrument/IDFObject.h"
#include <Poco/DateTimeFormatter.h>

namespace Mantid {
namespace Geometry {
//----------------------------------------------------------------------------------------------
/**
 * Returns the expected extension of an IDF file
 * @returns A string containing the expected extension of an IDF file, including
 * the leading period (.)
 */
const std::string AbstractIDFObject::expectedExtension() { return ".xml"; }

//----------------------------------------------------------------------------------------------
/** Constructor
*/
IDFObject::IDFObject(const std::string &fileName)
    : m_defFile(fileName), m_hasFileName(!fileName.empty()),
      m_cachePath(m_defFile.path()),
      m_cacheParentDirectory(m_cachePath.parent()),
      m_cachePathStr(m_cachePath.toString())

{}

//----------------------------------------------------------------------------------------------
/** Destructor
*/
IDFObject::~IDFObject() {}

/**
Gets the parent directory of the file.
@return Parent directory path.
*/
const Poco::Path &IDFObject::getParentDirectory() const {
  return m_cacheParentDirectory;
}

/**
Getter for the full file path.
@return Full file path.
*/
const Poco::Path &IDFObject::getFileFullPath() const { return m_cachePath; }

const std::string &IDFObject::getFileFullPathStr() const {
  return m_cachePathStr;
}

/**
Gets the filename for the FileObject.
@return filename only.
*/
std::string IDFObject::getFileNameOnly() const {
  return m_cachePath.getFileName();
}

/**
 * Gets the extension of this IDF file, including the leading period
 * @return A string containing the extension for this file
 */
std::string IDFObject::getExtension() const {
  std::string ext = m_cachePath.getExtension();
  if (ext.empty())
    return ext;
  else
    return "." + ext;
}

/**
Gets the last modified timestamp of the file.
@return last modified timestamp.
*/
Poco::Timestamp IDFObject::getLastModified() const {
  return m_defFile.getLastModified();
}

/**
Gets a formatted string of the last modified timestamp.
@return timestamp as a formatted string.
*/
std::string IDFObject::getFormattedLastModified() const {
  return Poco::DateTimeFormatter::format(this->getLastModified(),
                                         "%Y-%d-%mT%H:%M:%S");
}

/**
Gets the idf file as a mangled name.
@return the idf file as a mangled name.
*/
std::string IDFObject::getMangledName() const {
  return this->getFileNameOnly() + this->getFormattedLastModified();
}

/**
Check that the file exists.
@return True if it exists otherwise False.
*/
bool IDFObject::exists() const { return m_hasFileName && m_defFile.exists(); }

} // namespace Geometry
} // namespace Mantid
