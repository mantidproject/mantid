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

} // detail
} // IO
} // Parallell
} // Mantid
