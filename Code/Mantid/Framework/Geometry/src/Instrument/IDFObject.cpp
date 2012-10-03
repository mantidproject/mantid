#include "MantidGeometry/instrument/IDFObject.h"

namespace Mantid
{
  namespace Geometry
  {


    //----------------------------------------------------------------------------------------------
    /** Constructor
    */
    IDFObject::IDFObject(const std::string& fileName) : m_defFile(fileName)
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
      return m_defFile.exists();
    }




  } // namespace Geometry
} // namespace Mantid