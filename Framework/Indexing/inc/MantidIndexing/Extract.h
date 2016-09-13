#ifndef MANTID_INDEXING_EXTRACT_H_
#define MANTID_INDEXING_EXTRACT_H_

#include "MantidIndexing/DllConfig.h"

#include <vector>

namespace Mantid {
namespace Indexing {
class IndexInfo;

/** Extract : TODO: DESCRIPTION

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
MANTID_INDEXING_DLL IndexInfo
extract(const IndexInfo &source, const std::vector<size_t> &indices);
//IndexInfo extract(const IndexInfo &source, const std::vector<SpectrumNumber> &spectrumNumbers);
//IndexInfo extract(const IndexInfo &source, const std::vector<GlobalSpectrumIndex> &globalSpectrumIndices);


// Extraction object?

/*
class Extraction {
  public:
    Extraction(const IndexInfo &source, const std::vector<SpectrumNumber> &specNums);

    IndexInfo makeIndexInfo();
    std::vector<size_t> makeIndices();
};

Extraction extraction(ws.indexInfo(), {1,2,4,8});
auto ws2 = create<Workspace2D>(ws, extraction.makeIndexInfo());
const auto indices = extraction.makeIndices();
for (size_t i = 0; i < indices.size(), ++i) {
  ws2.setHistogram(i, ws.histogram(indices[i]));
}
*/




// Cases to cover are:
// - std::vector<SpectrumNumber>              -> IndexInfo  # basic construction
// - std::vector<SpectrumNumber>              -> SpectrumIndexSet
// - std::vector<SpectrumNumber, Data>        -> std::vector<size_t, Data>  # file loading (all load + filter)
// - std::vector<SpectrumNumber, Data>        -> std::vector<std::vector<size_t, Data>>  # file loading (load on master)
// - std::vector<SpectrumNumber>              -> IndexInfo, std::vector<size_t>  # aka extract
// - std::vector<std::vector<SpectrumNumber>> -> IndexInfo, std::vector<std::vector<size_t>>  # aka group
// - merging two or more objects (append support should be enough)


// SpectrumInfo defines grouping, should we avoid duplicating that info in IndexInfo?
// Can we avoid it? Where and how is translation between spectrum and detector used?
// Are all those cases eliminated by the introduction of SpectrumInfo?
//
// GroupDetectors:
// DetectorID -> SpectrumNumber (or index)
//
// class SpectrumInfo {
//   std::vector<std::vector<size_t>> m_detectorIndices;
// };
//
// If detector IDs are stored in IndexInfo, how would we validate them?
// - IndexInfo can validate SpectrumNumbers (right number, no duplicates)
// - Detector IDs can only be validated when we have access to instrument information:
//   * loading a file (ok with IndexInfo)
//   * creating new workspace based on existing one (ok with IndexInfo)
//   Ok, but somehow this feels as if a part of the instrument information is now stored in IndexInfo?
//
// SpectrumNumber <-> spectrum index translation in IndexInfo
// DetectorID <-> detector index translation in IndexInfo
// detector index <-> spectrum index in SpectrumInfo?
//
//
// or:
//
// Spectrum number <-> index in SpectrumInfo
// Detector ID <-> index in DetectorInfo
// grouping of detectors into spectra in SpectrumInfo
//
//
// global to local mapping?
//
// - do we need a global detector index?
//
//
// GlobalSpectrumNumberVector?
// or:
// how to avoid passing wrong objects to methods like extract?


/*
class Algorithm {
public:
  void declareSpectrumPropert();
  std::vector<size_t> getSpectrumIndicesFromProperty(const MatrixWorkspace &workspace);
  // but what about extract? It needs a vector of SpectrumNumber? (if we want to avoid unnecessary MPI comms.)
  std::vector<size_t> getIndexTranslatorFromProperty(); //?
};
*/


} // namespace Indexing
} // namespace Mantid

#endif /* MANTID_INDEXING_EXTRACT_H_ */
