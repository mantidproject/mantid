#ifndef MANTID_ISISREFLECTOMETRY_INDEXOF_H
#define MANTID_ISISREFLECTOMETRY_INDEXOF_H
#include <algorithm>
#include <boost/optional.hpp>
#include <iterator>

namespace MantidQt {
namespace CustomInterfaces {

template <typename Container, typename Predicate>
boost::optional<int> indexOf(Container const &container, Predicate pred) {
  auto maybeItemIt = std::find_if(container.cbegin(), container.cend(), pred);
  if (maybeItemIt != container.cend())
    return static_cast<int>(std::distance(container.cbegin(), maybeItemIt));
  else
    return boost::none;
}
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_INDEXOF_H
