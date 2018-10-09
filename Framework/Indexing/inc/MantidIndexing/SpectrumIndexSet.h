// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_INDEXING_SPECTRUMINDEXSET_H_
#define MANTID_INDEXING_SPECTRUMINDEXSET_H_

#include "MantidIndexing/DllConfig.h"
#include "MantidIndexing/IndexSet.h"

namespace Mantid {
namespace Indexing {

/** A set of spectrum indices, used for accessing spectra in a workspace. The
  indices in the set are validated and unique.

  @author Simon Heybrock
  @date 2017
*/
class MANTID_INDEXING_DLL SpectrumIndexSet
    : public detail::IndexSet<SpectrumIndexSet> {
public:
  using detail::IndexSet<SpectrumIndexSet>::IndexSet;
};

} // namespace Indexing
} // namespace Mantid

#endif /* MANTID_INDEXING_SPECTRUMINDEXSET_H_ */
