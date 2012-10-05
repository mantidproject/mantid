


//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidTestHelpers/ScopedFileHelper.h"

namespace ScopedFileHelper
{
    /**
    Constructor generates the file. Sets location to be the temp directory
    */
    ScopedFile::ScopedFile(const std::string& fileContents, const std::string& fileName) 
    {
      Poco::Path path(Mantid::Kernel::ConfigService::Instance().getTempDir().c_str());
      path.append(fileName);
      doCreateFile(fileContents, path);
    }

    /**
    Constructor generates the file. Sets location as defined by the fileDirectory.
    */
    ScopedFile::ScopedFile(const std::string& fileContents, const std::string& fileName, const std::string& fileDirectory)
    {
      Poco::Path path(fileDirectory);
      path.append(fileName);
      doCreateFile(fileContents, path);
    }

    /**
    Common method used by all constructors. Creates a file containing the ASCII file contents and 'remembers' the location of that file.
    */
    void ScopedFile::doCreateFile(const std::string& fileContents, const Poco::Path& fileNameAndPath)
    {
      m_filename = fileNameAndPath.toString();
      m_file.open (m_filename.c_str(), std::ios_base::out);
      if(!m_file.is_open())
      {
        throw std::runtime_error("Cannot open " + m_filename);
      }
      m_file << fileContents;
      m_file.close();
    }

    /**
    Getter for the filename
    @return File name only.
    */
    std::string ScopedFile::getFileName() const
    {
      return m_filename;
    }

    /// Free up resources.
    ScopedFile::~ScopedFile()
    {
      m_file.close();
      if( remove( m_filename.c_str() ) != 0 )
        throw std::runtime_error("cannot remove " + m_filename);
    }
}