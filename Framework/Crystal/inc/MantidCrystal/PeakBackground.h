#ifndef MANTID_CRYSTAL_PEAKBACKGROUND_H_
#define MANTID_CRYSTAL_PEAKBACKGROUND_H_

#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IPeaksWorkspace_fwd.h"
#include "MantidCrystal/HardThresholdBackground.h"
#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"
#include <boost/function.hpp>

namespace Mantid {
namespace Geometry {
class IPeak;
}
namespace Crystal {

/** PeakBackground : Extension of HardThresholdBackground to consider regions of
the image as background if they are outside
the peaks radius limits (no mater what their theshold is). For pixels inside the
radius, they must also be above the threshold value.

  Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

class DLLExport PeakBackground : public HardThresholdBackground {
private:
  /// Peak workspace containing peaks of interest
  Mantid::API::IPeaksWorkspace_const_sptr m_peaksWS;
  /// Radius estimate
  double m_radiusEstimate;
  /// MD coordinates to use
  Mantid::Kernel::SpecialCoordinateSystem m_mdCoordinates;
  /// Pointer to member function used for coordinate determination.
  boost::function<Mantid::Kernel::V3D(const Mantid::Geometry::IPeak *)>
      m_coordFunction;

public:
  /// Constructor
  PeakBackground(Mantid::API::IPeaksWorkspace_const_sptr peaksWS,
                 const double &radiusEstimate, const double &thresholdSignal,
                 const Mantid::API::MDNormalization normalisation,
                 const Mantid::Kernel::SpecialCoordinateSystem coordinates);

  /// Overriden is background function
  bool isBackground(Mantid::API::IMDIterator *iterator) const override;

  /// Overriden configure iterator function.
  void
  configureIterator(Mantid::API::IMDIterator *const iterator) const override;

  /// Virutal constructor
  PeakBackground *clone() const override;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_PEAKBACKGROUND_H_ */
