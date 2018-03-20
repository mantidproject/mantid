#ifndef MANTID_API_SPECTRUMDETECTORMAPPING_H_
#define MANTID_API_SPECTRUMDETECTORMAPPING_H_

#include <vector>
#include <set>
#ifndef Q_MOC_RUN
#include <unordered_map>
#endif

#include "MantidGeometry/IDTypes.h"
#include "MantidAPI/DllConfig.h"

namespace Mantid {
namespace API {
//----------------------------------------------------------------------
// Forward Declaration
//----------------------------------------------------------------------
class MatrixWorkspace;

/** A minimal class to hold the mapping between the spectrum number and its
   related
    detector ID numbers for a dataset.
    Normally, this mapping is contained within the collection of ISpectrum
   objects held
    by the workspace. This class can be useful when you want to pass just this
   information
    and not the entire workspace, or you want to store some mapping that related
   to spectra
    that are not yet, or no longer, contained in the workspace.

    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_API_DLL SpectrumDetectorMapping {
  using sdmap = std::unordered_map<specnum_t, std::set<detid_t>>;

public:
  explicit SpectrumDetectorMapping(const MatrixWorkspace *const workspace,
                                   bool useSpecNoIndex = true);
  SpectrumDetectorMapping(
      const std::vector<specnum_t> &spectrumNumbers,
      const std::vector<detid_t> &detectorIDs,
      const std::vector<detid_t> &ignoreDetIDs = std::vector<detid_t>());
  SpectrumDetectorMapping(const specnum_t *const spectrumNumbers,
                          const detid_t *const detectorIDs,
                          size_t arrayLengths);
  SpectrumDetectorMapping();
  virtual ~SpectrumDetectorMapping() = default;

  std::set<specnum_t> getSpectrumNumbers() const;
  const std::set<detid_t> &
  getDetectorIDsForSpectrumNo(const specnum_t spectrumNo) const;
  const std::set<detid_t> &
  getDetectorIDsForSpectrumIndex(const size_t spectrumIndex) const;
  const sdmap &getMapping() const;
  bool indexIsSpecNumber() const;

private:
  void fillMapFromArray(const specnum_t *const spectrumNumbers,
                        const detid_t *const detectorIDs,
                        const size_t arrayLengths);
  void fillMapFromVector(const std::vector<specnum_t> &spectrumNumbers,
                         const std::vector<detid_t> &detectorIDs,
                         const std::vector<detid_t> &ignoreDetIDs);

  bool m_indexIsSpecNo;
  /// The mapping of a spectrum number to zero or more detector IDs
  sdmap m_mapping;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_SPECTRUMDETECTORMAPPING_H_ */
