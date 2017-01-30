#ifndef MANTID_INDEXING_INDEXINFO_H_
#define MANTID_INDEXING_INDEXINFO_H_

#include "MantidIndexing/DllConfig.h"
#include "MantidIndexing/DetectorID.h"
#include "MantidIndexing/SpectrumNumber.h"
#include "MantidKernel/cow_ptr.h"

#include <functional>
#include <set>
#include <vector>

namespace Mantid {

<<<<<<< 208a5dde8dfafc5b56bfdaa2c35baa69748103b0
using specnum_t = int32_t;
using detid_t = int32_t;
=======
namespace Beamline {
>>>>>>> Re #18522. Use new type-safe detector IDs in spectrum numbers.
class SpectrumDefinition;

namespace Indexing {
class GlobalSpectrumIndex;
class SpectrumIndexSet;
class SpectrumNumberTranslator;

/** IndexInfo is an object for holding information about spectrum numbers and
  detector IDs associated to the spectra in a workspace.

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
  IndexInfo(std::vector<SpectrumNumber> &&spectrumNumbers,
            std::vector<std::vector<DetectorID>> &&detectorIDs);

  size_t size() const;

  SpectrumNumber spectrumNumber(const size_t index) const;
  const std::vector<DetectorID> &detectorIDs(const size_t index) const;

  void setSpectrumNumbers(std::vector<SpectrumNumber> &&spectrumNumbers);
  void setSpectrumNumbers(const SpectrumNumber min, const SpectrumNumber max);
  void setDetectorIDs(const std::vector<DetectorID> &detectorIDs);
  void setDetectorIDs(std::vector<std::vector<DetectorID>> &&detectorIDs);

  void setSpectrumDefinitions(
      Kernel::cow_ptr<std::vector<SpectrumDefinition>> spectrumDefinitions);
  const Kernel::cow_ptr<std::vector<SpectrumDefinition>> &
  spectrumDefinitions() const;

  SpectrumIndexSet makeIndexSet() const;
  SpectrumIndexSet makeIndexSet(SpectrumNumber min, SpectrumNumber max) const;
  SpectrumIndexSet makeIndexSet(GlobalSpectrumIndex min,
                                GlobalSpectrumIndex max) const;
  SpectrumIndexSet
  makeIndexSet(const std::vector<SpectrumNumber> &spectrumNumbers) const;
  SpectrumIndexSet
  makeIndexSet(const std::vector<GlobalSpectrumIndex> &globalIndices) const;

private:
  void makeSpectrumNumberTranslator();
  Kernel::cow_ptr<std::vector<SpectrumNumber>> m_spectrumNumbers;
  Kernel::cow_ptr<std::vector<std::vector<DetectorID>>> m_detectorIDs;
  Kernel::cow_ptr<std::vector<SpectrumDefinition>>
      m_spectrumDefinitions{nullptr};
  Kernel::cow_ptr<SpectrumNumberTranslator> m_spectrumNumberTranslator{nullptr};
};

} // namespace Indexing
} // namespace Mantid

#endif /* MANTID_INDEXING_INDEXINFO_H_ */
