#ifndef MANTID_BEAMLINE_SPECTRUMINFO_H_
#define MANTID_BEAMLINE_SPECTRUMINFO_H_

#include "MantidBeamline/DllConfig.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {
class SpectrumDefinition;
namespace Beamline {

/** Beamline::SpectrumInfo provides easy access to commonly used parameters of
  individual spectra (which may correspond to one or more detectors) in a
  beamline, such as mask and monitor flags, positions, L2, and 2-theta.

  Currently only a limited subset of functionality is implemented in
  Beamline::SpectrumInfo. The remainder is available in API::SpectrumInfo which
  acts as a wrapper around the old instrument implementation. API::SpectrumInfo
  will be removed once all functionality has been moved to
  Beamline::SpectrumInfo. For the time being, API::SpectrumInfo will forward
  calls to Beamline::SpectrumInfo when applicable.

  The reason for having both SpectrumInfo classes in parallel is:
  - We need to be able to move around the SpectrumInfo object including data it
    contains such as a vector of mask flags. This is relevant for the interface
    of ExperimentInfo, when replacing the ParameterMap or when setting a new
    instrument.
  - API::SpectrumInfo contains a caching mechanism and is frequently flushed
    upon modification of the instrument and is thus hard to handle outside the
    context of its owning workspace.
  Splitting SpectrumInfo into two classes seemed to be the safest and easiest
  solution to this.


  @author Simon Heybrock
  @date 2017

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_BEAMLINE_DLL SpectrumInfo {
public:
  SpectrumInfo(const size_t numberOfDetectors);
  SpectrumInfo(
      Kernel::cow_ptr<std::vector<SpectrumDefinition>> spectrumDefinition);

  size_t size() const;

  const SpectrumDefinition &spectrumDefinition(const size_t index) const;
  void setSpectrumDefinition(const size_t index, SpectrumDefinition def);
  const Kernel::cow_ptr<std::vector<SpectrumDefinition>> &
  sharedSpectrumDefinitions() const;

private:
  Kernel::cow_ptr<std::vector<SpectrumDefinition>> m_spectrumDefinition;
};

} // namespace Beamline
} // namespace Mantid

#endif /* MANTID_BEAMLINE_SPECTRUMINFO_H_ */
