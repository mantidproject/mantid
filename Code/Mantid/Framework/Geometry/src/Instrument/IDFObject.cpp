#include "MantidGeometry/Instrument/IDFObject.h"

namespace Mantid
{
  namespace Geometry
  {
    //----------------------------------------------------------------------------------------------
    /**
     * Returns the expected extension of an IDF file
     * @returns A string containing the expected extension of an IDF file, including the leading period (.)
     */
    const std::string IDFObject::expectedExtension()
    {
      return ".xml";
    }

    //----------------------------------------------------------------------------------------------
    /** Constructor
    */
    IDFObject::IDFObject(const std::string& fileName) : m_defFile(fileName), m_hasFileName(!fileName.empty())
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
    */
    IDFObject::~IDFObject()
    {
    }

    /**
    Gets the parent directory of the file.
    @return Parent directory path.
    */
    Poco::Path IDFObject::getParentDirectory() const
    {
      return Poco::Path(m_defFile.path()).parent();
    }

    /**
    Getter for the full file path.
    @return Full file path.
    */
    Poco::Path IDFObject::getFileFullPath() const
    {
      return m_defFile.path();
    }

    /**
    Gets the filename for the FileObject.
    @return filename only.
    */
    std::string IDFObject::getFileNameOnly() const
    {
      return Poco::Path(m_defFile.path()).getFileName();
    }

    /**
     * Gets the extension of this IDF file, including the leading period
     * @return A string containing the extension for this file
     */
    std::string IDFObject::getExtension() const
    {
      std::string ext = Poco::Path(m_defFile.path()).getExtension();
      if(ext.empty()) return ext;
      else return "." + ext;
    }

    /**
    Gets the last modified timestamp of the file.
    @return last modified timestamp.
    */
    Poco::Timestamp  IDFObject::getLastModified() const
    {
      return m_defFile.getLastModified();
    }

    /**
    Check that the file exists.
    @return True if it exists otherwise False.
    */
    bool IDFObject::exists() const
    {
      return m_hasFileName && m_defFile.exists();
    }




  } // namespace Geometry
} // namespace Mantid
