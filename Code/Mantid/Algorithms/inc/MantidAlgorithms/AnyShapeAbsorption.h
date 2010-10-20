#ifndef MANTID_ALGORITHMS_ANYSHAPEABSORPTION_H_
#define MANTID_ALGORITHMS_ANYSHAPEABSORPTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/AbsorptionCorrection.h"

namespace Mantid
{
namespace Algorithms
{
/** Calculates an approximation of the attenuation due to absorption and scattering in a 
    generic sample shape. The sample shape can be defined by, e.g., the CreateSampleShape
    algorithm. Note that the name of this algorithm is 'AbsorptionCorrection'.

    Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace. </LI>
    <LI> OutputWorkspace - The name of the output workspace. Can be the same as the input one. </LI>
    <LI> AttenuationXSection - The attenuation cross-section for the sample material in barns. </LI>
    <LI> ScatteringXSection - The scattering cross-section for the sample material in barns. </LI>
    <LI> SampleNumberDensity - The number density of the sample in Angstrom^-3.</LI>
    <LI> NumberOfWavelengthPoints - The number of wavelength points for which numerical integral is calculated (default: all points). </LI>
    <LI> ExpMethod - The method to calculate exponential function (Normal or Fast approximation). </LI>
    <LI> Emode  - The energy mode (0=elastic, 1=direct geometry, 2=indirect geometry) </LI>
    <LI> Efixed - Value of fixed energy: EI (emode=1) or EF (emode=2) (meV) </LI>
    <LI> ElementSize - The side dimension of an integration element cube in mm (default: 1). </LI>
    </UL>

    This algorithm uses numerical integration method to calculate attenuation factors
    resulting from absorption and single scattering in a sample with the material 
    properties given. Factors are calculated for each spectrum (i.e. detector position)
    and wavelength point, as defined by the input workspace. 
    The sample is first bounded by a cuboid, which is divided up into small cubes.
    The cubes whose centres lie within the sample make up the set of integration elements
    and path lengths through the sample are
    calculated for the centre-point of each element, and a numerical
    integration is carried out using these path lengths over the volume elements.

    This algorithm assumes that the beam comes along the Z axis
    and that the sample is at the origin.

    @author Russell Taylor, Tessella plc
    @date 11/03/2010

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
class DLLExport AnyShapeAbsorption : public AbsorptionCorrection
{
public:
  /// (Empty) Constructor
  AnyShapeAbsorption();
  /// Virtual destructor
  virtual ~AnyShapeAbsorption() {}
  /// Algorithm's name
  virtual const std::string name() const { return "AbsorptionCorrection"; }
  /// Algorithm's version
  virtual int version() const { return (1); }

private:
  void defineProperties();
  void retrieveProperties();
  std::string sampleXML();
  void initialiseCachedDistances();
  Geometry::Object constructGaugeVolume();

  double m_cubeSide;  ///< The length of the side of an element cube in m
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_ANYSHAPEABSORPTION_H_*/
