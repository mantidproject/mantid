#include "MantidKernel/FileDescriptor.h"
#include "MantidKernel/Exception.h"

#include <Poco/File.h>
#include <Poco/Path.h>

namespace Mantid
{
  namespace Kernel
  {
    //----------------------------------------------------------------------------------------------
    // Public methods
    //----------------------------------------------------------------------------------------------

    /**
     * @param filename A string containing a filename. The file must exist
     * @throws std::invalid_argument if the filename is empty or the file does not exist
     */
    FileDescriptor::FileDescriptor(const std::string & filename) :
        m_filename(), m_extension(), m_file(NULL)
    {
      if(filename.empty())
      {
        throw std::invalid_argument("FileDescriptor() - Empty filename '" + filename + "'");
      }
      if(!Poco::File(filename).exists())
      {
        throw std::invalid_argument("FileDescriptor() - File '" + filename + "' does not exist");
      }
      initialize(filename);
    }

    /**
     * Closes the file handle
     */
    FileDescriptor::~FileDescriptor()
    {
      m_file.close();
    }

    /**
     * Moves the stream pointer back to the start of the file, without
     * reopening the file. Note that this will affect the stream that
     * has been accessed using the stream() method
     */
    void FileDescriptor::resetStreamToStart()
    {
      m_file.seekg(0);
    }

    //----------------------------------------------------------------------------------------------
    // Private methods
    //----------------------------------------------------------------------------------------------

    /**
     * Set the description fields and opens the file
     * @param filename A string pointing to an existing file
     */
    void FileDescriptor::initialize(const std::string& filename)
    {
      m_filename = filename;
      m_extension = "." + Poco::Path(filename).getExtension();

      m_file.open(m_filename.c_str(), std::ios::in | std::ios::binary);
      if(!m_file) throw std::runtime_error("FileDescriptor::initialize - Cannot open file '" + filename + "' for reading");
    }

  } // namespace Kernel
} // namespace Mantid
