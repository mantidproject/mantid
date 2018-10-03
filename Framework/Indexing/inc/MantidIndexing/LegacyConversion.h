// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_INDEXING_LEGACYCONVERSION_H_
#define MANTID_INDEXING_LEGACYCONVERSION_H_

#include "MantidIndexing/DllConfig.h"
#include "MantidIndexing/SpectrumNumber.h"

#include <vector>

namespace Mantid {
namespace Indexing {

/** This header provides convenience methods for converting from and to legacy
  index types such as specnum_t (int32_t).

  @author Simon Heybrock
  @date 2017
*/
MANTID_INDEXING_DLL std::vector<SpectrumNumber>
makeSpectrumNumberVector(const std::vector<int32_t> &data);

} // namespace Indexing
} // namespace Mantid

#endif /* MANTID_INDEXING_LEGACYCONVERSION_H_ */
