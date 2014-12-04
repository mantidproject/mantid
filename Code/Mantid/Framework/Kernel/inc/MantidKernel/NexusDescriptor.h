#ifndef MANTID_KERNEL_NEXUSDESCRIPTOR_H_
#define MANTID_KERNEL_NEXUSDESCRIPTOR_H_

#include "MantidKernel/ClassMacros.h"
#include "MantidKernel/DllConfig.h"

#include <map>
#include <set>
#include <string>
#include <utility>

namespace NeXus
{
  class File;
}

namespace Mantid
{
  namespace Kernel
  {

    /** 
        Defines a wrapper around a file whose internal structure can be accessed using the NeXus API

        On construction the simple details about the layout of the file are cached for faster querying later.

        Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

        This file is part of Mantid.

        Mantid is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 3 of the License, or
        (at your option) any later version.

        Mantid is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program.  If not, see <http://www.gnu.org/licenses/>.

        File change history is stored at: <https://github.com/mantidproject/mantid>
        Code Documentation is available at: <http://doxygen.mantidproject.org>
     */
    class MANTID_KERNEL_DLL NexusDescriptor
    {
    public:
      /// Enumerate HDF possible versions
      enum Version { Version4, Version5, AnyVersion };

      static const size_t HDFMagicSize;
      /// HDF cookie that is stored in the first 4 bytes of the file.
      static const unsigned char HDFMagic[4];
      /// Size of HDF5 signature
      static size_t HDF5SignatureSize;
      /// signature identifying a HDF5 file.
      static const unsigned char HDF5Signature[8];

      /// Returns true if the file is considered to store data in a hierarchy
      static bool isHDF(const std::string & filename, const Version version = AnyVersion);

    public:
      /// Constructor accepting a filename
      NexusDescriptor(const std::string & filename);
      /// Destructor
      ~NexusDescriptor();

      /**
       * Access the filename
       * @returns A reference to a const string containing the filename
       */
      inline const std::string & filename() const { return m_filename; }
      /**
       * Access the file extension. Defined as the string after and including the last period character
       * @returns A reference to a const string containing the file extension
       */
      inline const std::string & extension() const { return m_extension; }
      /**
       * Access the open NeXus File object
       * @returns A reference to the open ::NeXus file object
       */
      inline ::NeXus::File & data() { return *m_file; }

      /// Returns the name & type of the first entry in the file
      const std::pair<std::string,std::string> & firstEntryNameType() const;
      /// Query if the given attribute exists on the root node
      bool hasRootAttr(const std::string &name) const;
      /// Query if a path exists
      bool pathExists(const std::string& path) const;
      /// Query if a path exists of a given type
      bool pathOfTypeExists(const std::string& path, const std::string &type) const;
      /// return the path of a given type
      std::string pathOfType(const std::string & type) const;
      /// Query if a given type exists somewhere in the file
      bool classTypeExists(const std::string & classType) const;

    private:
      DISABLE_DEFAULT_CONSTRUCT(NexusDescriptor);
      DISABLE_COPY_AND_ASSIGN(NexusDescriptor);

      /// Initialize object with filename
      void initialize(const std::string& filename);
      /// Walk the tree and cache the structure
      void walkFile(::NeXus::File & file, const std::string & rootPath, const std::string & className,
                    std::map<std::string, std::string> & pmap, int level);

      /// Full filename
      std::string m_filename;
      /// Extension
      std::string m_extension;
      /// First entry name/type
      std::pair<std::string, std::string> m_firstEntryNameType;
      /// Root attributes
      std::set<std::string> m_rootAttrs;
      /// Map of full path strings to types. Can check if path exists quickly
      std::map<std::string, std::string> m_pathsToTypes;

      /// Open NeXus handle
      ::NeXus::File *m_file;
    };


  } // namespace Kernel
} // namespace Mantid

#endif  /* MANTID_KERNEL_HIERARCHICALFILEDESCRIPTOR_H_ */
