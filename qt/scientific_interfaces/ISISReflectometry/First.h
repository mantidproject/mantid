#ifndef MANTID_ISISREFLECTOMETRY_FIRST_H
#define MANTID_ISISREFLECTOMETRY_FIRST_H
#include <vector>
#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {

template <typename T>
boost::optional<T> first(std::vector<T> const &values) {
  if (values.size() > 0)
    return values[0];
  else {
    return boost::none;
  }
}

}
}
#endif // MANTID_ISISREFLECTOMETRY_FIRST_H
