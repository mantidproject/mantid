#ifndef MANTID_KERNEL_INSTRUMENTINFO_H_
#define MANTID_KERNEL_INSTRUMENTINFO_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/LiveListenerInfo.h"

#include <map>
#include <set>
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

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
class FacilityInfo;

/** A class that holds information about an instrument.

    Copyright &copy; 2007-2012 STFC Rutherford Appleton Laboratory

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
class MANTID_KERNEL_DLL InstrumentInfo {
public:
  /// Constructor
  InstrumentInfo(const FacilityInfo *f, const Poco::XML::Element *elem);
  /// Equality operator
  bool operator==(const InstrumentInfo &rhs) const;
  /// Return the name of the instrument
  const std::string name() const;
  /// Return the short name of the instrument
  const std::string shortName() const;
  /// Returns zero padding for this instrument and a run number
  int zeroPadding(unsigned int runNumber) const;
  /// Returns file prefix for this instrument and a run number
  std::string filePrefix(unsigned int runNumber) const;
  /// Returns the default delimiter between instrument name and run number
  std::string delimiter() const;
  /// Return list of techniques
  const std::set<std::string> &techniques() const;
  /// The facility to which this instrument belongs
  const FacilityInfo &facility() const;

  /// Returns the name of the default live listener
  std::string liveListener(const std::string &name = "") const;
  /// Returns a string containing the "host:port" for default live listener
  std::string liveDataAddress(const std::string &name = "") const;
  /// Returns LiveListenerInfo for specified connection name (or default)
  const LiveListenerInfo &liveListenerInfo(std::string name = "") const;
  /// Returns true if this instrument has at least one live listener defined
  bool hasLiveListenerInfo() const;
  /// Returns all available LiveListenerInfos as a vector
  const std::vector<LiveListenerInfo> &liveListenerInfoList() const;

private:
  void fillTechniques(const Poco::XML::Element *elem);
  void fillLiveData(const Poco::XML::Element *elem);
  void fillZeroPadding(const Poco::XML::Element *elem);

  /// Typedef for the zeropadding holder, first is starting run-number,
  /// second is file prefix - zero padding pair
  using ZeroPaddingMap = std::map<unsigned int, std::pair<std::string, int>>;
  /// get the zeropadding part
  int getZeroPadding(ZeroPaddingMap::const_iterator it) const {
    return it->second.second;
  }
  /// get the prefix part
  const std::string &getPrefix(ZeroPaddingMap::const_iterator it) const {
    return it->second.first;
  }

  const FacilityInfo *m_facility; ///< Facility
  std::string m_name;             ///< Instrument name
  std::string m_shortName;        ///< Instrument short name
  ZeroPaddingMap m_zeroPadding;   ///< Run number-dependent zero padding
  std::string m_delimiter; ///< Delimiter between instrument name and run number
  std::set<std::string>
      m_technique; ///< List of techniques the instrument can do

  std::vector<LiveListenerInfo> m_listeners; ///< LiveListener connections
  std::string m_defaultListener; ///< Default LiveListener connection to use
};

/// Allow this object to be printed to a stream
MANTID_KERNEL_DLL std::ostream &
operator<<(std::ostream &buffer, const InstrumentInfo &instrumentDescriptor);

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_INSTRUMENTINFO_H_ */
