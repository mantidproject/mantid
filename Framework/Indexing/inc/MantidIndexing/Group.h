// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_INDEXING_GROUP_H_
#define MANTID_INDEXING_GROUP_H_

#include "MantidIndexing/DllConfig.h"
#include "MantidIndexing/SpectrumNumber.h"

#include <vector>

namespace Mantid {
namespace Indexing {
class IndexInfo;

/** Functions for grouping spectra. A new IndexInfo with the desired grouping is
  created based on an existing one.

  @author Simon Heybrock
  @date 2016
*/
MANTID_INDEXING_DLL IndexInfo
group(const IndexInfo &source, std::vector<SpectrumNumber> &&specNums,
      const std::vector<std::vector<std::size_t>> &grouping);

} // namespace Indexing
} // namespace Mantid

#endif /* MANTID_INDEXING_GROUP_H_ */
