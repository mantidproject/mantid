#ifndef MANTID_ALGORITHMS_CUBOIDGAUGEVOLUMEABSORPTION_H_
#define MANTID_ALGORITHMS_CUBOIDGAUGEVOLUMEABSORPTION_H_

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
  std::string sampleXML();
  void initialiseCachedDistances();
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_FLATPLATEABSORPTION_H_*/
