// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Instrument/DetectorInfoItem.h"
#include "MantidGeometry/Instrument/InfoIteratorBase.h"

namespace Mantid {
namespace Geometry {

/** DetectorInfoIterator for random access iteration over DetectorInfo

Random access iterator for iteration over DetectorInfo
@author Bhuvan Bezawada, STFC
@date 2018
*/
template <typename T> class DetectorInfoIterator : public InfoIteratorBase<T, DetectorInfoItem> {
public:
  using InfoIteratorBase<T, DetectorInfoItem>::InfoIteratorBase;
};
} // namespace Geometry
} // namespace Mantid
