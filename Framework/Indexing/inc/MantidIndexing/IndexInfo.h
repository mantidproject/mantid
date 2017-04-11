#ifndef MANTID_INDEXING_INDEXINFO_H_
#define MANTID_INDEXING_INDEXINFO_H_

#include "MantidIndexing/DllConfig.h"
#include "MantidKernel/cow_ptr.h"

#include <functional>
#include <set>
#include <vector>

namespace Mantid {

using specnum_t = int32_t;
using detid_t = int32_t;
class SpectrumDefinition;

namespace Indexing {

/** IndexInfo is an object for holding information about spectrum numbers and
  detector IDs associated to the spectra in a workspace.

  Currently this class supports a legacy "wrapper mode": Spectrum numbers and
  detector IDs are still stored inside the ISpectrums that are part of a
  MatrixWorkspace. Ultimately this data will be moved into IndexInfo. For the
  time being, while the old interface still exists, this cannot be done.
  Instead, for any IndexInfo object that is part of a MatrixWorkspace, the
  methods in IndexInfo will provide access to data stored in the associated
  MatrixWorkspace (or the respective ISpectrum objects). The m_isLegacy flag
  indicates that an instance of IndexInfo is in such a wrapping state. Taking a
  copy of IndexInfo will cause a transition from this wrapping legacy state to a
  stand-alone state without associated MatrixWorkspace.

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
class MANTID_INDEXING_DLL IndexInfo {
public:
  explicit IndexInfo(const size_t globalSize);
  IndexInfo(std::vector<specnum_t> &&spectrumNumbers,
            std::vector<std::vector<detid_t>> &&detectorIDs);
  IndexInfo(
      std::function<size_t()> getSize,
      std::function<specnum_t(const size_t)> getSpectrumNumber,
      std::function<const std::set<specnum_t> &(const size_t)> getDetectorIDs);

  IndexInfo(const IndexInfo &other);

  size_t size() const;

  specnum_t spectrumNumber(const size_t index) const;
  std::vector<detid_t> detectorIDs(const size_t index) const;

  void setSpectrumNumbers(std::vector<specnum_t> &&spectrumNumbers) & ;
  void setDetectorIDs(const std::vector<detid_t> &detectorIDs) & ;
  void setDetectorIDs(std::vector<std::vector<detid_t>> &&detectorIDs) & ;

  void setSpectrumDefinitions(
      Kernel::cow_ptr<std::vector<SpectrumDefinition>> spectrumDefinitions);
  const Kernel::cow_ptr<std::vector<SpectrumDefinition>> &
  spectrumDefinitions() const;

private:
  /// True if class is legacy wrapper.
  bool m_isLegacy{false};
  /// Function bound to MatrixWorkspace::getNumbersHistograms(), used only in
  /// legacy mode.
  std::function<size_t()> m_getSize;
  /// Function bound to MatrixWorkspace::spectrumNumber(), used only in legacy
  /// mode.
  std::function<specnum_t(const size_t)> m_getSpectrumNumber;
  /// Function bound to MatrixWorkspace::detectorIDs(), used only in legacy
  /// mode.
  std::function<const std::set<specnum_t> &(const size_t)> m_getDetectorIDs;

  Kernel::cow_ptr<std::vector<specnum_t>> m_spectrumNumbers;
  Kernel::cow_ptr<std::vector<std::vector<detid_t>>> m_detectorIDs;

  Kernel::cow_ptr<std::vector<SpectrumDefinition>> m_spectrumDefinitions{
      nullptr};
};

} // namespace Indexing
} // namespace Mantid

#endif /* MANTID_INDEXING_INDEXINFO_H_ */
