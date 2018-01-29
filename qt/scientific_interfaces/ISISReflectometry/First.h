#ifndef MANTID_ISISREFLECTOMETRY_FIRST_H
#define MANTID_ISISREFLECTOMETRY_FIRST_H
#include <vector>
#include <boost/optional.hpp>
#include <boost/variant.hpp>

namespace MantidQt {
namespace CustomInterfaces {

template <typename T> boost::optional<T> first(std::vector<T> const &values) {
  if (values.size() > 0)
    return boost::optional<T>(values[0]);
  else
    return boost::none;
}

template <typename... Ts> class FirstVisitor : boost::static_visitor<> {
public:
  template <typename T>
  boost::optional<boost::variant<Ts...>>
  operator()(std::vector<T> const &values) const {
    auto value = first(values);
    if (value)
      return boost::variant<Ts...>(value.get());
    else
      return boost::none;
  }
};
}
}
#endif // MANTID_ISISREFLECTOMETRY_FIRST_H
