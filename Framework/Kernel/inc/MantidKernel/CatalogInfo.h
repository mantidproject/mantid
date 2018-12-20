#ifndef MANTID_KERNEL_CATALOGINFO_H_
#define MANTID_KERNEL_CATALOGINFO_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/ICatalogInfo.h"
#include <string>

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
namespace Poco {
namespace XML {
class Element;
}
} // namespace Poco

namespace Mantid {
namespace Kernel {

/** A class that holds information about catalogs.

    Copyright &copy; 2007-2013 ISIS Rutherford Appleton Laboratory, NScD Oak
   Ridge National Laboratory & European Spallation Source

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

class MANTID_KERNEL_DLL CatalogInfo : public ICatalogInfo {
public:
  /// Constructor
  CatalogInfo(const Poco::XML::Element *element);
  /// Copy constructor
  CatalogInfo(const CatalogInfo &other);
  /// Obtain catalog name from the facility file.
  const std::string catalogName() const override;
  /// Obtain soap end point from the facility file.
  const std::string soapEndPoint() const override;
  /// Obtain the external download URL.
  const std::string externalDownloadURL() const override;
  /// Obtain the regex prefix from the  facility file.
  const std::string catalogPrefix() const override;
  /// Obtain Windows prefix from the facility file.
  const std::string windowsPrefix() const override;
  /// Obtain Macintosh prefix from facility file.
  const std::string macPrefix() const override;
  /// Obtain Linux prefix from facility file.
  const std::string linuxPrefix() const override;
  /// Clone
  CatalogInfo *clone() const override;

private:
  /// Obtain the attribute from a given element tag and attribute name.
  std::string getAttribute(const Poco::XML::Element *element,
                           const std::string &tagName,
                           const std::string &attributeName);

  // Disabled assignment operator.
  CatalogInfo &operator=(const CatalogInfo &other);

  std::string m_catalogName;
  std::string m_soapEndPoint;
  std::string m_externalDownloadURL;
  std::string m_catalogPrefix;
  std::string m_windowsPrefix;
  std::string m_macPrefix;
  std::string m_linuxPrefix;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_CATALOGINFO_H_ */
