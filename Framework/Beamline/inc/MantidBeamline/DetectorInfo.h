#ifndef MANTID_BEAMLINE_DETECTORINFO_H_
#define MANTID_BEAMLINE_DETECTORINFO_H_

#include "MantidBeamline/DllConfig.h"

namespace Mantid {
namespace Beamline {

/** Beamline::DetectorInfo provides easy access to commonly used parameters of
  individual detectors (pixels) in a beamline, such as mask and monitor flags,
  positions, L2, and 2-theta.

  Currently only a limited subset of functionality is implemented in
  Beamline::DetectorInfo. The remainder is available in API::DetectorInfo which
  acts as a wrapper around the old instrument implementation. API::DetectorInfo
  will be removed once all functionality has been moved to
  Beamline::DetectorInfo. For the time being, API::DetectorInfo will forward
  calls to Beamline::DetectorInfo when applicable.

  The reason for having both DetectorInfo classes in parallel is:
  - We need to be able to move around the DetectorInfo object including data it
    contains such as a vector of mask flags. This is relevant for the interface
    of ExperimentInfo, when replacing the ParameterMap or when setting a new
    instrument.
  - API::DetectorInfo contains a caching mechanism and is frequently flushed
    upon modification of the instrument and is thus hard to handle outside the
    context of its owning workspace.
  Splitting DetectorInfo into two classes seemed to be the safest and easiest
  solution to this.


  @author Simon Heybrock
  @date 2016

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_BEAMLINE_DLL DetectorInfo {
public:
  DetectorInfo(const size_t numberOfDetectors);

  size_t size() const;

private:
  size_t m_size;
};

} // namespace Beamline
} // namespace Mantid

#endif /* MANTID_BEAMLINE_DETECTORINFO_H_ */
