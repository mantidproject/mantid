// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidParallel/IO/NXEventDataLoader.h"

namespace Mantid {
namespace Parallel {
namespace IO {
namespace detail {

std::string readAttribute(const H5::DataSet &dataSet,
                          const std::string &attributeName) {
  const auto &attr = dataSet.openAttribute(attributeName);
  std::string value;
  attr.read(attr.getDataType(), value);
  return value;
}

} // namespace detail
} // namespace IO
} // namespace Parallel
} // namespace Mantid
