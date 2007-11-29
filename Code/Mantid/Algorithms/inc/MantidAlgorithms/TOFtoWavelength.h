#ifndef MANTID_TOFTOWAVELENGTH_H_
#define MANTID_TOFTOWAVELENGTH_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** @class TOFtoWavelength TOFtoWavelength.h Algorithms/TOFtoWavelength.h

    Converts all of the TOF axes in a 2D workspace to wavelength values.
    The algorithm is: h / wavelength = m_n * (d1+d2)/tof
    where m_n is the mass of a neutron, d1 is the distance from the source to the sample,
    d2 is the distance from the sample to the detector & tof is the time of flight. 

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the workspace to take as input </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the result </LI>
    </UL>
    
    @author Russell Taylor, Tessella Support Services plc
    @date 30/10/2007
    
    Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>    
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport TOFtoWavelength : public API::Algorithm
{
public:
  /// Default constructor
	TOFtoWavelength();
	/// Destructor
	virtual ~TOFtoWavelength();

private:
  // Overridden Algorithm methods  
  Kernel::StatusCode init();
  Kernel::StatusCode exec();
  Kernel::StatusCode final();
  
  /// Static reference to the logger class
  static Kernel::Logger& g_log;
  };

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_TOFTOWAVELENGTH_H_*/
