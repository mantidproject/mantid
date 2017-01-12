#ifndef MANTID_BEAMLINE_SPECTRUMDEFINITION_H_
#define MANTID_BEAMLINE_SPECTRUMDEFINITION_H_

#include "MantidBeamline/DllConfig.h"

#include <vector>

namespace Mantid {
namespace Beamline {

/** SpectrumDefinition : TODO: DESCRIPTION

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
class MANTID_BEAMLINE_DLL SpectrumDefinition {
public:
  size_t size() const;
  const std::pair<size_t, size_t> &operator[](const size_t index) const;
  void add(const size_t detectorIndex, const size_t timeIndex = 0);

private:
  std::vector<std::pair<size_t, size_t>> m_data;

public:
  auto begin() const -> decltype(m_data.begin()) { return m_data.begin(); }
  auto end() const -> decltype(m_data.end()) { return m_data.end(); }
  auto cbegin() const -> decltype(m_data.cbegin()) { return m_data.cbegin(); }
  auto cend() const -> decltype(m_data.cend()) { return m_data.cend(); }
};

} // namespace Beamline
} // namespace Mantid

#endif /* MANTID_BEAMLINE_SPECTRUMDEFINITION_H_ */
