#ifndef MANTID_KERNEL_FACILITYINFO_H_
#define MANTID_KERNEL_FACILITYINFO_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/CatalogInfo.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/RemoteJobManager.h"
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif
#include <vector>
#include <string>

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
namespace Poco {
namespace XML {
class Element;
}
}

namespace Mantid {
namespace Kernel {

/** A class that holds information about a facility.

    Copyright &copy; 2007-2012 ISIS Rutherford Appleton Laboratory, NScD Oak
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
  const std::vector<std::string> extensions() const { return m_extensions; }
  /// Returns the preferred file extension
  const std::string &preferredExtension() const { return m_extensions.front(); }

  /// Return the archive search interface names
  const std::vector<std::string> &archiveSearch() const {
    return m_archiveSearch;
  }
  /// Returns the name of the default live listener
  const std::string &liveListener() const { return m_liveListener; }
  /// Returns a list of instruments of this facility
  const std::vector<InstrumentInfo> &instruments() const {
    return m_instruments;
  }
  /// Returns a list of instruments of given technique
  std::vector<InstrumentInfo> instruments(const std::string &tech) const;
  /// Returns instruments with given name
  const InstrumentInfo &instrument(std::string iName = "") const;
  /// Returns a vector of the available compute resources
  std::vector<std::string> computeResources() const;
  /// Returns the RemoteJobManager for the named compute resource
  boost::shared_ptr<RemoteJobManager>
  getRemoteJobManager(const std::string &name) const;
  /// Returns the catalogInfo class.
  const CatalogInfo &catalogInfo() const { return m_catalogs; }

private:
  void fillZeroPadding(const Poco::XML::Element *elem);
  void fillDelimiter(const Poco::XML::Element *elem);
  void fillExtensions(const Poco::XML::Element *elem);
  void fillArchiveNames(const Poco::XML::Element *elem);
  void fillInstruments(const Poco::XML::Element *elem);
  void fillLiveListener(const Poco::XML::Element *elem);
  void fillHTTPProxy(const Poco::XML::Element *elem);
  void fillComputeResources(const Poco::XML::Element *elem);

  /// Add new extension
  void addExtension(const std::string &ext);

  CatalogInfo m_catalogs;   ///< Gain access to the catalogInfo class.
  const std::string m_name; ///< facility name
  int m_zeroPadding;        ///< default zero padding for this facility
  std::string
      m_delimiter; ///< default delimiter between instrument name and run number
  std::vector<std::string>
      m_extensions; ///< file extensions in order of preference
  std::vector<std::string>
      m_archiveSearch; ///< names of the archive search interface
  std::vector<InstrumentInfo>
      m_instruments;          ///< list of instruments of this facility
  std::string m_liveListener; ///< name of the default live listener
  typedef std::map<std::string, boost::shared_ptr<RemoteJobManager>>
      ComputeResourcesMap;
  ComputeResourcesMap m_computeResources; ///< list of compute resources
                                          ///(clusters, etc...) available at
  /// this facility
  // (Sorted by their names)
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_FACILITYINFO_H_ */
