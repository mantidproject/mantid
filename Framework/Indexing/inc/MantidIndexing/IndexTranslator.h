#ifndef MANTID_INDEXING_INDEXTRANSLATOR_H_
#define MANTID_INDEXING_INDEXTRANSLATOR_H_

#include "MantidIndexing/DllConfig.h"
#include "MantidKernel/cow_ptr.h"

#include <vector>

namespace Mantid {

using specnum_t = int32_t;
using detid_t = int32_t;

namespace Indexing {

/** IndexTranslator : TODO: DESCRIPTION

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
class MANTID_INDEXING_DLL IndexTranslator {
public:
  // Create default translator. size is global size
  // Default implies 1:1 spectrum numbers and detector IDs, each defined as
  // (global) workspace index + 1
  //
  // Can we internally provide an optimization for the case of trivial mapping?
  // We want to avoid complicated maps if it is just a simple offset, i.e.,
  // SpectrumNumber = WorkspaceIndex + 1 (will always be more complex with MPI?).
  explicit IndexTranslator(const size_t globalSize);

  /// The *local* size, i.e., the number of spectra in this partition.
  size_t size() const;

  specnum_t spectrumNumber(const size_t index) const {
    return (*m_spectrumNumbers)[index];
  }

  std::vector<detid_t> detectorIDs(const size_t index) const {
    return (*m_detectorIDs)[index];
  }

  // maybe this must be fixed at construction time? makes setting spectrum numbers etc. easier.
  // but how do we create a translator from an existing one with different partitioning?
  //
  // are we storing enough information for being able to change partitioning?
  // currently no: we do not have information about ordering of spectrum numbers
  // (m_specNumToPartition is unordered)
  // void setPartitioning(const Partitioning &partitioning) &;

  // arguments are for all ranks
  // usually this builds maps, finds out which indices are local, ...
  void setSpectrumNumbers(std::vector<specnum_t> &&spectrumNumbers) &;
  //void setSpectrumNumbers(std::initializer_list<specnum_t> &&ilist) &;

  void setDetectorIDs(const std::vector<detid_t> &detectorIDs) &;
  void setDetectorIDs(std::vector<std::vector<detid_t>> &&detectorIDs) &;
  //void setDetectorIDs(std::vector<std::vector<detid_t>> &&detectorIDs) &;
  //void setDetectorIDs(std::initializer_list<detid_t> &&ilist) &;

  // Do we have this info? Build from info in spectra. But this would be
  // *local*? Yes, can do better only once we moved things out of ISpectrum.
  //
  // Do we want/need reference access?
  //
  // for all ranks? this is not what clients would expect?
  //
  // where is this needed?
  // MatrixWorkspace can use it to set spectrum Numbers in ISpectra
  // this must return only *local* spectrum numbers
  //std::vector<specnum_t> makeSpectrumNumberVector() const;
  //std::vector<specnum_t>
  //makeSpectrumNumberVector(const std::vector<size_t> &indices) const;
  //SpectrumIndexSet makeSpectrumIndexSet() const;
  //SpectrumIndexSet makeSpectrumIndexSet(specnum_t min, specnum_t max) const;

  // How to obtain a single spectrum number?
  //specnum_t spectrumNumber(const size_t index) const;
  //std::vector<detid_t> detectorIDs(const size_t index) const;

private:
  // temporarily: friend class MatrixWorkspace?
  //const std::vector<specnum_t> &spectrumNumbers() const &;
  //const std::vector<std::vector<detid_t>> &detectorIDs() const &;

  Kernel::cow_ptr<std::vector<specnum_t>> m_spectrumNumbers;
  Kernel::cow_ptr<std::vector<std::vector<detid_t>>> m_detectorIDs;

  // unordered_map m_specNumToPartition
  // map m_specNumToLocal
  // map m_globalToLocal // can stay shared when changing specNums
};

} // namespace Indexing
} // namespace Mantid


/*
auto translator = ws.translator();
translator.setSpectrumNumbers({1,2,3,4});
ws.setTranslator(translator);

Translator MatrixWorkspace::translator() {
  // Workaround while Translator is not store in MatrixWorkspace:
  // build translator based on data in ISpectra
  Translator t(getNumberHistograms());
  std::vector<specnum_t> specNums;
  std::vector<std::vector<specnum_t>> specNums;
  for(size_t i=0; i<getNumberHistograms(); ++i) {
    const auto &spec = getSpectrum(i);
    specNums.push_back(spec.getSpectrumNo());
    detIDs.push_back(spec.getDetectorIDs());
  }
  t.setSpectrumNumbers(std::move(specNums));
  t.setDetectorIDs(std::move(detIDs));
  return t;
}

void MatrixWorkspace::setTranslator(const Translator &) {
  // Workaround see MatrixWorkspace.cpp

  // Later:
  // How do we validate detector IDs? Can we require specific ordering of
  // setting Translator and setting Instrument?
  //
  // Translator defines detectorID -> detector index.
  // Once we have indices, would we even notice bad detector IDs? Instrument does not know about IDs. Problems occur only when user input does not match information in Translator.
  // That is, it may happen that things develop a mismatch between data in an
  // input file and a workspace. However, this would not break anything, apart
  // from displaying "wrong" detector IDs and having output files with wrong
  // detector IDs. Is this just a renaming?
  //
  // If translator changes spectrum, redistribution may be required. Can that be
  // part of this routine? It looks like a complex task that might live
  // somewhere else?
  //
}

// sharing data in translators
auto translator = inputWS.translator();
outputWS.setTranslator(translator);
*/


#endif /* MANTID_INDEXING_INDEXTRANSLATOR_H_ */
