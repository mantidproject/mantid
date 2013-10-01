#ifndef MANTID_KERNEL_CATALOGINFO_H_
#define MANTID_KERNEL_CATALOGINFO_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/Logger.h"

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
namespace Poco
{
  namespace XML
  {
    class Element;
  }
}

namespace Mantid
{
  namespace Kernel
  {

  /** A class that holds information about a catalogs.

      Copyright &copy; 2007-2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

      File change history is stored at: <https://github.com/mantidproject/mantid>.
      Code Documentation is available at: <http://doxygen.mantidproject.org>
   */

    class MANTID_KERNEL_DLL CatalogInfo
    {
      public:
        /// Obtain catalog name from the facility file.
        std::string catalogName(const Poco::XML::Element* element);
        /// Obtain soap end point from the facility file.
        std::string soapEndPoint(const Poco::XML::Element* element);
        /// Obtain Windows prefix from the facility file.
        std::string windowsPrefix(const Poco::XML::Element* element);
        /// Obtain Macintosh prefix from facility file.
        std::string macPrefix(const Poco::XML::Element* element);
        /// Obtain Linux prefix from facility file.
        std::string linuxPrefix(const Poco::XML::Element* element);
        /// Transform's the archive path based on operating system used.
        std::string transformArchivePath(std::string& path);
        /// Replaces backward slash with forward slashes for Unix compatibility.
        void replaceBackwardSlash(std::string& path);
    };

  } // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_CATALOGINFO_H_ */
