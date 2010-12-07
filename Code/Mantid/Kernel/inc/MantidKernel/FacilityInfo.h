#ifndef MANTID_KERNEL_FACILITYINFO_H_
#define MANTID_KERNEL_FACILITYINFO_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllExport.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/InstrumentInfo.h"

#include <vector>
#include <string>

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

/** A class that holds information about a facility.

    @author Roman Tolchenov, Tessella plc
    @date 20/07/2010

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class EXPORT_OPT_MANTID_KERNEL FacilityInfo
{
public:
  FacilityInfo(const Poco::XML::Element* elem);
  /// Return the name of the facility
  const std::string name()const{return m_name;}
  /// Returns default zero padding for this facility
  int zeroPadding()const{return m_zeroPadding;}
  /// Returns the default delimiter between instrument name and run number
  const std::string delimiter() const{return m_delimiter;}
  /// Returns a list of file extensions
  const std::vector<std::string> extensions()const{return m_extensions;}
  /// Returns the preferred file extension
  const std::string preferredExtension()const{return m_extensions.front();}
  /// Return the archive search interface names
  const std::set<std::string>& archiveSearch()const{return m_archiveSearch;}
  /// Returns a list of instruments of this facility
  const std::vector<InstrumentInfo>& Instruments()const{return m_instruments;}
  /// Returns a list of instruments of given technique
  std::vector<InstrumentInfo> Instruments(const std::string& tech)const;
  /// Returns instruments with given name
  const InstrumentInfo & Instrument(const std::string& iName = "")const;

  ///returns the catalog name
  const std::string  catalogName()const {return m_catalogName;}
private:
  ///Default constructor
  FacilityInfo();
  /// Add new extension
  void addExtension(const std::string& ext);

  const std::string m_name;                    ///< facility name
  int m_zeroPadding;                           ///< default zero padding for this facility
  std::string m_delimiter;                     ///  default delimiter between instrument name and run number
  std::vector<std::string> m_extensions;       ///< file extensions in order of preference
  std::set<std::string> m_archiveSearch;       ///< names of the archive search interface
  std::vector<InstrumentInfo> m_instruments;   ///< list of instruments of this facility
  std::string m_catalogName;                   ///< name of the catalog system of this facilty
  static Logger& g_log;                        ///< logger
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_FACILITYINFO_H_ */
