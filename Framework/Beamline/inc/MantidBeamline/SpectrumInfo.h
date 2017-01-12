#ifndef MANTID_BEAMLINE_SPECTRUMINFO_H_
#define MANTID_BEAMLINE_SPECTRUMINFO_H_

#include "MantidBeamline/DllConfig.h"
#include "MantidKernel/cow_ptr.h"

#include <vector>

namespace Mantid {
namespace Beamline {
class SpectrumDefinition;

/** SpectrumInfo : TODO: DESCRIPTION

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

  size_t size() const;

  const SpectrumDefinition &spectrumDefinition(const size_t index) const;
  void setSpectrumDefinition(const size_t index, SpectrumDefinition def);

private:
  Kernel::cow_ptr<std::vector<SpectrumDefinition>> m_spectrumDefinition;
};

} // namespace Beamline
} // namespace Mantid

#endif /* MANTID_BEAMLINE_SPECTRUMINFO_H_ */
