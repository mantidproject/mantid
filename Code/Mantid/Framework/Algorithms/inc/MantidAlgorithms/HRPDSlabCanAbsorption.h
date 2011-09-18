#ifndef MANTID_ALGORITHMS_HRPDSLABCANABSORPTION_H_
#define MANTID_ALGORITHMS_HRPDSLABCANABSORPTION_H_
/*WIKI* 


This algorithm is a refinement of the [[FlatPlateAbsorption]] algorithm for the specific case of an HRPD 'slab can' sample holder. It uses the aforementioned generic algorithm to calculate the correction due to the sample itself, using numerical integration. This is done using the standard height x width dimensions of an HRPD sample holder of 23 x 18 mm. Valid values of the thickness are 2,5,10 & 15 mm, although this is not currently enforced.

Further corrections are then carried out to account for the 0.125mm Vanadium windows at the front and rear of the sample, and for the aluminium of the holder itself (which is traversed by neutrons en route to the 90 degree bank). This is carried out using an analytical approximation for a flat plate, the correction factor being calculated as
<math> \rm{exp} \left( \frac{- \rho \left( \sigma_a \frac{ \lambda} {1.798} + \sigma_s \right) t}{\rm{cos} \, \theta} \right) </math>, where <math>\lambda</math> is the wavelength, <math>\theta</math> the angle between the detector and the normal to the plate and the other symbols are as given in the property list above. The assumption is that the neutron enters the plate along the normal.

==== Restrictions on the input workspace ====
The input workspace must have units of wavelength. The [[instrument]] associated with the workspace must be fully defined because detector, source & sample position are needed.

===Subalgorithms used===
The [[FlatPlateAbsorption]] algorithm is used to calculate the correction due to the sample itself.



*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** Calculates attenuation due to absorption and scattering in an HRPD 'slab' can

  Properties:
  <UL>
  <LI> InputWorkspace  - The name of the input workspace. </LI>
  <LI> OutputWorkspace - The name of the output workspace. Can be the same as the input one. </LI>
  <LI> SampleAttenuationXSection - The attenuation cross-section for the sample material in barns. </LI>
  <LI> SampleScatteringXSection - The scattering cross-section for the sample material in barns. </LI>
  <LI> SampleNumberDensity - The number density of the sample in Angstrom^-3.</LI>
  <LI> NumberOfWavelengthPoints - The number of wavelength points for which numerical integral is calculated (default: all points). </LI>
  <LI> ExpMethod - The method to calculate exponential function (Normal or Fast approximation). </LI>
  <LI> Thickness - The thickness of the sample holder in centimetres. </LI>
  <LI> ElementSize - The side dimension of an integration element cube in mm (default: 1). </LI>
  </UL>

  This algorithm uses the FlatPlateAbsorption algorithm to numerically calculate the
  correction for the sample itself, and then further modifies the correction to account
  for the vanadium windows at the front and rear of the sample holder, and the traversal
  of the holder material (aluminium) itself en route to the 90 degree bank.

  The further corrections calculated within this algorithm use an analytical approximation
  of the form exp(-mt/cos(theta)).

  @author Russell Taylor, Tessella plc
  @date 05/02/2010

  Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport HRPDSlabCanAbsorption : public API::Algorithm
{
public:
  /// (Empty) Constructor
  HRPDSlabCanAbsorption() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~HRPDSlabCanAbsorption() {}
  /// Algorithm's name
  virtual const std::string name() const { return "HRPDSlabCanAbsorption"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Diffraction"; }

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  /// Initialisation code
  void init();
  /// Execution code
  void exec();

  ///runs the flat plate absorption
  API::MatrixWorkspace_sptr runFlatPlateAbsorption();
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_HRPDSLABCANABSORPTION_H_*/
