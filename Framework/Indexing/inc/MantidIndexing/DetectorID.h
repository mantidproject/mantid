// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_INDEXING_DETECTORID_H_
#define MANTID_INDEXING_DETECTORID_H_

#include "MantidIndexing/DllConfig.h"
#include "MantidIndexing/IndexType.h"

namespace Mantid {
namespace Indexing {

/** Unique ID for a detector in a beamline.

  @author Simon Heybrock
  @date 2017
*/
class DetectorID : public detail::IndexType<DetectorID, int32_t> {
public:
  using detail::IndexType<DetectorID, int32_t>::IndexType;
  using detail::IndexType<DetectorID, int32_t>::operator=;
};

} // namespace Indexing
} // namespace Mantid

#endif /* MANTID_INDEXING_DETECTORID_H_ */
