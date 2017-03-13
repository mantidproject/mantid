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
class SpectrumDefinition;
namespace Indexing {
class GlobalSpectrumIndex;
class SpectrumIndexSet;
class SpectrumNumberTranslator;

/** IndexInfo provides mapping from spectrum numbers to spectrum indices, and
  grouping information that defines a spectrum as a group of detectors.

  The interface of IndexInfo is designed to hide an underlying partitioning of
  data as in the case of MPI. There are three interconnected index types:
  - Spectrum numbers are user-defined (instrument-specific) identifiers for a
    spectrum. In principle these must be unique, but for legacy support this is
    currently not guaranteed. Most of the key functionality of IndexInfo is not
    available unless spectrum numbers are unique.
  - Global spectrum indices are a contiguous way of indexing all spectra,
    starting at zero. In particular, this index spans all partitions. If there
    is only a single partition the global spectrum index is equivalent to the
    index (see next item). Note that in the user interface this is termed
    `workspace index`.
  - A contiguous index that is used to access data in workspaces. This index
    refers only to spectra on this partition and is thus used in all client code
    when accessing a partitioned workspace.
  Typically, input from users or files would be in terms of spectrum numbers or
  global spectrum indices. IndexInfo is then used to translate these into a set
  of indices, whereby IndexInfo internally takes care of including all indices
  in question in the set, such that the union of sets on all partitions
  corresponds to the requested spectrum numbers or global spectrum indices.
  Client code that treats each spectrum on its own can thus be written without
  concern or knowledge about the underlying partitioning of the data.

  Note that currently IndexInfo also carries information about grouping in terms
  of detector IDs. This mechanism has been added for supporting the current
  legacy in Mantid: Grouping is stored in terms of detector IDs in ISpectrum (in
  addition to the new way of storing grouping with the class SpectrumDefinition
  in terms of indices). We also store detector IDs such that grouping
  information can be transferred to another workspace, without the need for a
  costly (and potentially impossible, since invalid IDs are allowed) rebuild of
  the sets of detector IDs that need to be part of ISpectrum.


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
  void makeSpectrumNumberTranslator() const;
  Kernel::cow_ptr<std::vector<SpectrumNumber>> m_spectrumNumbers;
  Kernel::cow_ptr<std::vector<std::vector<DetectorID>>> m_detectorIDs;
  Kernel::cow_ptr<std::vector<SpectrumDefinition>> m_spectrumDefinitions{
      nullptr};
  mutable Kernel::cow_ptr<SpectrumNumberTranslator> m_spectrumNumberTranslator{
      nullptr};
};

} // namespace Indexing
} // namespace Mantid

#endif /* MANTID_INDEXING_INDEXINFO_H_ */
