#include "MantidKernel/HDFDescriptor.h"
#include "MantidKernel/Exception.h"

#include <nexus/NeXusFile.hpp>
#include <nexus/NeXusException.hpp>

#include <Poco/File.h>
#include <Poco/Path.h>

#include <cstring>

namespace Mantid
{
  namespace Kernel
  {
    //---------------------------------------------------------------------------------------------------------------------------
    // static HDFDescriptor constants
    //---------------------------------------------------------------------------------------------------------------------------
    /// Size of HDF magic number
    const size_t HDFDescriptor::HDFMagicSize = 4;
    /// HDF cookie that is stored in the first 4 bytes of the file.
    const unsigned char HDFDescriptor::HDFMagic[4] = {'\016','\003','\023','\001'}; // From HDF4::hfile.h

    /// Size of HDF5 signature
    size_t HDFDescriptor::HDF5SignatureSize = 8;
    /// signature identifying a HDF5 file.
    const unsigned char HDFDescriptor::HDF5Signature[8] = { 137, 'H', 'D', 'F', '\r', '\n', '\032', '\n' };

    namespace
    {
      //---------------------------------------------------------------------------------------------------------------------------
      // Anonymous helper methods to use isHDF methods to use an open file handle
      //---------------------------------------------------------------------------------------------------------------------------

      /**
       * Currently simply checks for the HDF signatures and returns true if one of them is found
       * @param fileHandle A file handled opened and pointing at the start of the file. On return the
       * fileHandle is left at the start of the file
       * @param version One of the HDFDescriptor::Version enumerations specifying the required version
       * @return True if the file is considered hierarchical, false otherwise
       */
      bool isHDFHandle(FILE *fileHandle, HDFDescriptor::Version version)
      {
        if(!fileHandle) throw std::invalid_argument("HierarchicalFileDescriptor::isHierarchical - Invalid file handle");

        bool result(false);

        // HDF4 check requires 4 bytes,  HDF5 check requires 8 bytes
        // Use same buffer and waste a few bytes if only checking HDF4
        unsigned char buffer[8] = {'0','0','0','0','0','0','0','0'};
        std::fread(static_cast<void*>(&buffer), sizeof(unsigned char), HDFDescriptor::HDF5SignatureSize, fileHandle);
        // Number of bytes read doesn't matter as if it is not enough then the memory simply won't match
        // as the buffer has been "zeroed"
        if(version == HDFDescriptor::Version5 || version == HDFDescriptor::AnyVersion )
        {
          result = (std::memcmp(&buffer, &HDFDescriptor::HDF5Signature, HDFDescriptor::HDF5SignatureSize) == 0);
        }
        if(!result && (version == HDFDescriptor::Version4 || version == HDFDescriptor::AnyVersion) )
        {
          result = (std::memcmp(&buffer, &HDFDescriptor::HDFMagic, HDFDescriptor::HDFMagicSize) == 0);
        }

        // Return file stream to start of file
        std::rewind(fileHandle);
        return result;
      }
    }

    //---------------------------------------------------------------------------------------------------------------------------
    // static HDFDescriptor methods
    //---------------------------------------------------------------------------------------------------------------------------

    /**
     * Checks for the HDF signatures and returns true if one of them is found
     * @param filename A string filename to check
     * @param version One of the HDFDescriptor::Version enumerations specifying the required version
     * @return True if the file is considered hierarchical, false otherwise
     */
    bool HDFDescriptor::isHDF(const std::string & filename, const Version version)
    {
      FILE *fd = fopen(filename.c_str(), "rb");
      if(!fd)
      {
        throw std::invalid_argument("HierarchicalFileDescriptor::isHierarchical - Unable to open file '" + filename + "'");
      }
      const bool result = isHDFHandle(fd, version); // use anonymous helper
      fclose(fd);
      return result;
    }

    //---------------------------------------------------------------------------------------------------------------------------
    // HDFDescriptor public methods
    //---------------------------------------------------------------------------------------------------------------------------
    /**
     * Constructs the wrapper
     * @param filename A string pointing to an existing file
     * @throws std::invalid_argument if the file is not identified to be hierarchical. This currently
     * involves simply checking for the signature if a HDF file at the start of the file
     */
    HDFDescriptor::HDFDescriptor(const std::string & filename)
      : m_filename(), m_extension()
    {
      if(filename.empty())
      {
        throw std::invalid_argument("HDFDescriptor() - Empty filename '" + filename + "'");
      }
      if(!Poco::File(filename).exists())
      {
        throw std::invalid_argument("HDFDescriptor() - File '" + filename + "' does not exist");
      }
      initialize(filename);
    }

    /**
     * @param path A string giving a path using UNIX-style path separators (/), e.g. /raw_data_1, /entry/bank1
     * @return True if the path exists in the file, false otherwise
     */
    bool HDFDescriptor::pathExists(const std::string& path) const
    {
      return true;
    }

    //---------------------------------------------------------------------------------------------------------------------------
    // HDFDescriptor private methods
    //---------------------------------------------------------------------------------------------------------------------------

    /**
     * Creates the internal cached structure of the file as a tree of nodes
     */
    void HDFDescriptor::initialize(const std::string& filename)
    {
      m_filename = filename;
      m_extension = "." + Poco::Path(filename).getExtension();

      try
      {
        ::NeXus::File file(this->filename());
      }
      catch(::NeXus::Exception &)
      {
        throw std::invalid_argument("HDFDescriptor::initialize - File '" + filename + "' does not look like a HDF file.");
      }
//      // Root node has no type and is named "/"
//      m_root->name = "/";
//
//      addChildren(file, "/", m_root);
//

//      auto rootEntries = file.getEntries();
//      for(auto it = rootEntries.begin(); rootEntries.end(); ++it)
//      {
//        auto node = boost::make_shared<Node>();
//        node->name = it->first;
//        node->type = it->second;
//        m_roots.insert(std::make_pair(it->first, node));
//      }
    }


  } // namespace Kernel
} // namespace Mantid
