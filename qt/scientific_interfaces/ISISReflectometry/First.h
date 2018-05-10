#ifndef MANTID_ISISREFLECTOMETRY_FIRST_H
#define MANTID_ISISREFLECTOMETRY_FIRST_H
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

template <typename T> boost::optional<T> first(std::vector<T> const &values) {
  if (values.size() > 0)
    return boost::optional<T>(values[0]);
  else
    return boost::none;
}

/**
 * Operates on a variant<vector<Ts>...> extracting the first element and
 * returning it as a optional<variant<T>> where the optional is empty if
 * the vector held by the variant<vector<T>> held no values.
 */
template <typename... Ts>
class FirstVisitor
    : public boost::static_visitor<boost::optional<boost::variant<Ts...>>> {
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
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_FIRST_H
