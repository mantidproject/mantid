// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
 */

class MANTID_KERNEL_DLL CatalogInfo : public ICatalogInfo {
public:
  /// Constructor
  CatalogInfo(const Poco::XML::Element *element);
  /// Copy constructor
  CatalogInfo(const CatalogInfo &other) = default;
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
  std::string getAttribute(const Poco::XML::Element *element, const std::string &tagName,
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
