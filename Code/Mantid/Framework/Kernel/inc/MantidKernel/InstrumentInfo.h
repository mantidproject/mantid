#ifndef MANTID_KERNEL_INSTRUMENTINFO_H_
#define MANTID_KERNEL_INSTRUMENTINFO_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllExport.h"
#include "MantidKernel/Logger.h"

#include <set>
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

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
class FacilityInfo;

/** A class that holds information about an instrument.

    @author Roman Tolchenov, Tessella plc
    @date 21/07/2010

    Copyright &copy; 2007-9 STFC Rutherford Appleton Laboratory

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
class EXPORT_OPT_MANTID_KERNEL InstrumentInfo
{
public:
  /// Constructor
  InstrumentInfo(FacilityInfo* f,const Poco::XML::Element* elem);
  /// Equality operator
  bool operator==(const InstrumentInfo & rhs) const;
  /// Return the name of the instrument
  const std::string name() const;
  /// Return the short name of the instrument
  const std::string shortName() const ;
  /// Returns zero padding for this instrument
  int zeroPadding() const;
  /// Returns the default delimiter between instrument name and run number
  std::string delimiter() const;
  /// Return list of techniques
  const std::set<std::string>& techniques() const;
  const FacilityInfo& facility() const;
private:
  const FacilityInfo* m_facility;          ///< facility
  std::string m_name;                      ///< instrument name
  std::string m_shortName;                 ///< instrument short name
  int m_zeroPadding;                       ///< default zero padding for this facility
  std::string m_delimiter;                 ///  default delimiter between instrument name and run number
  std::set<std::string> m_technique;       ///< list of techniques the instrument can do
  static Logger& g_log;                    ///< logger
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_INSTRUMENTINFO_H_ */
