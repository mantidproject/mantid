// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_COMPONENTINFOITERATOR_H_
#define MANTID_GEOMETRY_COMPONENTINFOITERATOR_H_

#include "MantidGeometry/Instrument/ComponentInfoItem.h"
#include "MantidGeometry/Instrument/InfoIteratorBase.h"

namespace Mantid {
namespace Geometry {

/** ComponentInfoIterator for random access iteration over ComponentInfo
 */
template <typename T>
class ComponentInfoIterator : public InfoIteratorBase<T, ComponentInfoItem> {
public:
  using InfoIteratorBase<T, ComponentInfoItem>::InfoIteratorBase;
};
} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_COMPONENTINFOITERATOR_H_ */
