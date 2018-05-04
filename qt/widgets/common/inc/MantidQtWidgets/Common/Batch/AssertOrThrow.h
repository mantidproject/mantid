#ifndef MANTIDQTMANTIDWIDGETS_ASSERTORTHROW_H_
#define MANTIDQTMANTIDWIDGETS_ASSERTORTHROW_H_
#include <string>
#include <stdexcept>
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

inline void assertOrThrow(bool condition, std::string const &message) {
  if (!condition)
    throw std::runtime_error(message);
}
}
}
}
#endif
