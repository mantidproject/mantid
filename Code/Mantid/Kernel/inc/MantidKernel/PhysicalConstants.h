#ifndef MANTID_PHYSICALCONSTANTS_H_
#define MANTID_PHYSICALCONSTANTS_H_

#ifndef M_PI 
#define M_PI 3.1415926535897932384626433832795 
#endif

namespace Mantid
{

/** A namespace containing physical constants that are required by algorithms and unit routines.

    @author Russell Taylor, Tessella Support Services plc
    @date 30/10/2007
    
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
namespace PhysicalConstants
{
  /** Planck constant. Taken from <http://physics.nist.gov/cuu/Constants> on 31/10/2007. */
  static const double h = 6.62606896e-34;

  /** Standard acceleration due to gravity. Precise value in ms<SUP>-2</SUP>. */
  static const double g = 9.80665;
  
  /** Mass of the neutron in kg. Taken from <http://physics.nist.gov/cuu/Constants> on 30/10/2007. */
  static const double NeutronMass = 1.674927211e-27;
  
  /** 1 meV in Joules. Taken from <http://physics.nist.gov/cuu/Constants> on 28/03/2008. */
  static const double meV = 1.602176487e-22;
  
  /** 1 meV in wavenumber (cm<SUP>-1</SUP>). Taken from <http://physics.nist.gov/cuu/Constants> on 02/04/2008. */
  static const double meVtoWavenumber = 8.06554465;
	
  /**  Muon lifetime. Taken from MuLan experiment on 08/12/2008. */
  static const double MuonLifetime = 2.197019e-6;

  /** Standard atmospheric pressure in kPa. 
   * Taken from <http://physics.nist.gov/cgi-bin/cuu/Value?stdatm|search_for=adopted_in!> on 01/12/2010 **/
  static const double StandardAtmosphere = 101.325;
  
} // namespace PhysicalConstants
} // namespace Mantid

#endif /*MANTID_PHYSICALCONSTANTS_H_*/
