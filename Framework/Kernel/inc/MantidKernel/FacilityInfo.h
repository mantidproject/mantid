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
#include "MantidKernel/CatalogInfo.h"
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/InstrumentInfo.h"
#ifndef Q_MOC_RUN
#include <memory>
#endif
#include <string>
#include <vector>

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

/** A class that holds information about a facility.
 */

class MANTID_KERNEL_DLL FacilityInfo {
public:
  explicit FacilityInfo(const Poco::XML::Element *elem);

  /// Return the name of the facility
  const std::string &name() const { return m_name; }

  /// Returns default zero padding for this facility
  int zeroPadding() const { return m_zeroPadding; }
  /// Returns the default delimiter between instrument name and run number
  const std::string &delimiter() const { return m_delimiter; }
  /// Returns a list of file extensions
  const std::vector<std::string> &extensions() const { return m_extensions; }
  /// Returns the preferred file extension
  const std::string &preferredExtension() const { return m_extensions.front(); }

  /// Returns the time zone designation compatible with pytz
  const std::string &timezone() const { return m_timezone; }

  /// Return the archive search interface names
  const std::vector<std::string> &archiveSearch() const { return m_archiveSearch; }
  /// Returns a list of instruments of this facility
  const std::vector<InstrumentInfo> &instruments() const { return m_instruments; }
  /// Returns a list of instruments of given technique
  std::vector<InstrumentInfo> instruments(const std::string &tech) const;
  /// Returns instruments with given name
  const InstrumentInfo &instrument(std::string iName = "") const;

  /// Returns the catalogInfo class.
  const CatalogInfo &catalogInfo() const { return m_catalogs; }

  /// Returns a bool indicating whether prefix is required in file names
  bool noFilePrefix() const { return m_noFilePrefix; }

  /// Returns the multiple file limit
  size_t multiFileLimit() const { return m_multiFileLimit; }

private:
  void fillZeroPadding(const Poco::XML::Element *elem);
  void fillDelimiter(const Poco::XML::Element *elem);
  void fillExtensions(const Poco::XML::Element *elem);
  void fillArchiveNames(const Poco::XML::Element *elem);
  void fillTimezone(const Poco::XML::Element *elem);
  void fillInstruments(const Poco::XML::Element *elem);
  void fillHTTPProxy(const Poco::XML::Element *elem);
  void fillNoFilePrefix(const Poco::XML::Element *elem);
  void fillMultiFileLimit(const Poco::XML::Element *elem);

  /// Add new extension
  void addExtension(const std::string &ext);

  CatalogInfo m_catalogs;                    ///< Gain access to the catalogInfo class.
  const std::string m_name;                  ///< facility name
  std::string m_timezone;                    ///< Timezone designation in pytz
  int m_zeroPadding;                         ///< default zero padding for this facility
  std::string m_delimiter;                   ///< default delimiter between instrument name and run number
  std::vector<std::string> m_extensions;     ///< file extensions in order of preference
  std::vector<std::string> m_archiveSearch;  ///< names of the archive search interface
  std::vector<InstrumentInfo> m_instruments; ///< list of instruments of this facility
  bool m_noFilePrefix;                       ///< flag indicating if prefix is required in file names
  size_t m_multiFileLimit;                   ///< the multiple file limit
};

} // namespace Kernel
} // namespace Mantid
