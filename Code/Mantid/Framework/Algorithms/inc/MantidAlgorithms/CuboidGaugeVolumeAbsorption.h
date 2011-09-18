#ifndef MANTID_ALGORITHMS_CUBOIDGAUGEVOLUMEABSORPTION_H_
#define MANTID_ALGORITHMS_CUBOIDGAUGEVOLUMEABSORPTION_H_
/*WIKI* 

This algorithm uses a numerical integration method to calculate attenuation factors resulting from absorption and single scattering within a cuboid region of a sample with the dimensions and material properties given. '''The gauge volume generated will be an axis-aligned cuboid centred on the sample (centre) position. The sample must fully enclose this cuboid. If this does not meet your needs you can instead use the general [[AbsorptionCorrection]] algorithm in conjunction with [[DefineGaugeVolume]].'''

Factors are calculated for each spectrum (i.e. detector position) and wavelength point, as defined by the input workspace. The sample is divided up into cuboids having sides of as close to the size given in the ElementSize property as the sample dimensions will allow. Thus the calculation speed depends linearly on the total number of bins in the workspace and goes as <math>\rm{ElementSize}^{-3}</math>.

Path lengths through the sample are then calculated for the centre-point of each element and a numerical integration is carried out using these path lengths over the volume elements.

==== Restrictions on the input workspace ====
The input workspace must have units of wavelength. The [[instrument]] associated with the workspace must be fully defined because detector, source & sample position are needed. A sample shape must have been defined using, e.g., [[CreateSampleShape]] and the gauge volume must be fully within the sample.



*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FlatPlateAbsorption.h"

namespace Mantid
{
namespace Algorithms
{
/** Calculates attenuation due to absorption and scattering in a generic sample, considering only the
    scattering within a cuboid shaped 'gauge volume'.

    This gauge volume will be an axis-aligned cuboid with its centre at the samplePos point. The
    sample object must have been previously defined and must fully enclose the gauge volume.

    @author Russell Taylor, Tessella
    @date 1/11/2010

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
class DLLExport CuboidGaugeVolumeAbsorption : public FlatPlateAbsorption
{
public:
  /// (Empty) Constructor
  CuboidGaugeVolumeAbsorption();
  /// Virtual destructor
  virtual ~CuboidGaugeVolumeAbsorption() {}
  /// Algorithm's name
  virtual const std::string name() const { return "CuboidGaugeVolumeAbsorption"; }
  /// Algorithm's version
  virtual int version() const { return (1); }

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  std::string sampleXML();
  void initialiseCachedDistances();
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_FLATPLATEABSORPTION_H_*/
