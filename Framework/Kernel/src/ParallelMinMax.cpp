#include "MantidKernel/ParallelMinMax.h"

#include <utility>
#include <vector>

namespace Mantid::Kernel {

namespace {
/** MinMaxFinder
 * A templated functor for use in parallel_minmax, which will search a vector to find a min/max over a subrange.
 * These are then joined together for total min/max values
 */
template <typename T> class MANTID_KERNEL_DLL MinMaxFinder {
  std::vector<T> const *vec;

public:
  T minval;
  T maxval;

  // copy min/max from the other. we're all friends
  MinMaxFinder(MinMaxFinder<T> &other, tbb::split) : vec(other.vec), minval(other.minval), maxval(other.maxval) {}

  // set the min=max=first element supplied
  MinMaxFinder(std::vector<T> const *vec) : vec(vec), minval(vec->front()), maxval(vec->front()) {}

  void operator()(tbb::blocked_range<size_t> const &range) {
    const auto [minele, maxele] = std::minmax_element(vec->cbegin() + range.begin(), vec->cbegin() + range.end());
    if (*minele < minval)
      minval = *minele;
    if (*maxele > maxval)
      maxval = *maxele;
  }

  void join(MinMaxFinder<T> const &other) {
    if (other.minval < minval)
      minval = other.minval;
    if (other.maxval > maxval)
      maxval = other.maxval;
  }
};
} // namespace

template <typename T> std::pair<T, T> parallel_minmax(std::vector<T> const *const vec, size_t const grainsize) {
  if (vec->size() < grainsize) {
    const auto [minval, maxval] = std::minmax_element(vec->cbegin(), vec->cend());
    return std::make_pair(*minval, *maxval);
  } else {
    MinMaxFinder<T> finder(vec);
    tbb::parallel_reduce(tbb::blocked_range<size_t>(0, vec->size(), grainsize), finder);
    return std::make_pair(finder.minval, finder.maxval);
  }
}

template <typename T>
std::pair<T, T> parallel_minmax(std::shared_ptr<std::vector<T>> const &vec, size_t const grainsize) {
  return parallel_minmax<T>(vec.get(), grainsize);
}

template <typename T>
std::pair<T, T> parallel_minmax(std::unique_ptr<std::vector<T>> const &vec, size_t const grainsize) {
  return parallel_minmax<T>(vec.get(), grainsize);
}

#define EXPORTPARALLELMINMAX(type)                                                                                     \
  template std::pair<type, type> MANTID_KERNEL_DLL parallel_minmax(std::shared_ptr<std::vector<type>> const &,         \
                                                                   size_t);                                            \
  template std::pair<type, type> MANTID_KERNEL_DLL parallel_minmax(std::unique_ptr<std::vector<type>> const &, size_t);

EXPORTPARALLELMINMAX(float)
EXPORTPARALLELMINMAX(double)
EXPORTPARALLELMINMAX(uint32_t)

#undef EXPORTPARALLELMINMAX

} // namespace Mantid::Kernel
