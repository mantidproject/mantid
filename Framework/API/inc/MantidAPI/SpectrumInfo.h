#ifndef MANTID_API_SPECTRUMINFO_H_
#define MANTID_API_SPECTRUMINFO_H_

#include "MantidAPI/DllConfig.h"

#include <memory>

namespace Mantid {
namespace API {

class GeometryInfo;
class GeometryInfoFactory;
class MatrixWorkspace;

/** API::SpectrumInfo is an intermediate step towards a SpectrumInfo that is
  part of Instrument-2.0. The aim is to provide a nearly identical interface
  such that we can start refactoring existing code before the full-blown
  implementation of Instrument-2.0 is available.

  This class is thread safe with OpenMP BUT NOT WITH ANY OTHER THREADING LIBRARY
  such as Poco threads or Intel TBB.


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
class MANTID_API_DLL SpectrumInfo {
public:
  SpectrumInfo(const MatrixWorkspace &workspace);
  SpectrumInfo(SpectrumInfo &&other);
  ~SpectrumInfo();

  bool isMonitor(const size_t index) const;
  bool isMasked(const size_t index) const;

private:
  const GeometryInfo &getInfo(const size_t index) const;
  void updateCachedInfo(const size_t index) const;
  std::unique_ptr<GeometryInfoFactory> m_factory;
  mutable std::vector<std::unique_ptr<GeometryInfo>> m_info;
  mutable std::vector<size_t> m_lastIndex;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_SPECTRUMINFO_H_ */
