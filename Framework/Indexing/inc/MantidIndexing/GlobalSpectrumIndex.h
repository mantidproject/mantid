// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_INDEXING_GLOBALSPECTRUMINDEX_H_
#define MANTID_INDEXING_GLOBALSPECTRUMINDEX_H_

#include "MantidIndexing/DllConfig.h"
#include "MantidIndexing/IndexType.h"

#include <cstddef>

namespace Mantid {
namespace Indexing {

/** A global index for spectra. The index starts at 0 and is contiguous, i.e.,
  spectra have global indices in the range 0...N_spectra-1.

  @author Simon Heybrock
  @date 2017
*/
class GlobalSpectrumIndex
    : public detail::IndexType<GlobalSpectrumIndex, size_t> {
public:
  using detail::IndexType<GlobalSpectrumIndex, size_t>::IndexType;
  using detail::IndexType<GlobalSpectrumIndex, size_t>::operator=;
};

} // namespace Indexing
} // namespace Mantid

#endif /* MANTID_INDEXING_GLOBALSPECTRUMINDEX_H_ */
