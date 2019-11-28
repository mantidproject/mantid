// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_INDEXING_EXTRACT_H_
#define MANTID_INDEXING_EXTRACT_H_

#include "MantidIndexing/DllConfig.h"

#include <vector>

namespace Mantid {
namespace Indexing {
class IndexInfo;
class SpectrumIndexSet;

/** Functions for extracting spectra. A new IndexInfo with the desired spectra
  is created based on an existing one.

  @author Simon Heybrock
  @date 2016
*/
MANTID_INDEXING_DLL IndexInfo extract(const IndexInfo &source,
                                      const SpectrumIndexSet &indices);
MANTID_INDEXING_DLL IndexInfo extract(const IndexInfo &source,
                                      const std::vector<std::size_t> &indices);
MANTID_INDEXING_DLL IndexInfo extract(const IndexInfo &source,
                                      const std::size_t minIndex,
                                      const std::size_t maxIndex);

} // namespace Indexing
} // namespace Mantid

#endif /* MANTID_INDEXING_EXTRACT_H_ */
