#include "MantidKernel/ParallelMinMax.h"

#include <utility>
#include <vector>

namespace Mantid::Kernel {

template <typename T> void MinMaxFinder<T>::operator()(tbb::blocked_range<size_t> const &range) {
  const auto [minele, maxele] = std::minmax_element(vec->cbegin() + range.begin(), vec->cbegin() + range.end());
  if (*minele < minval)
    minval = *minele;
  if (*maxele > maxval)
    maxval = *maxele;
}

template <typename T> void MinMaxFinder<T>::join(MinMaxFinder<T> const &other) {
  if (other.minval < minval)
    minval = other.minval;
  if (other.maxval > maxval)
    maxval = other.maxval;
}

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

#define EXPORTPARALLELMINMAX(type)                                                                                     \
  template std::pair<type, type> MANTID_KERNEL_DLL parallel_minmax(std::vector<type> const *const, size_t);

EXPORTPARALLELMINMAX(double)
EXPORTPARALLELMINMAX(uint32_t)

#undef EXPORTPARALLELMINMAX

} // namespace Mantid::Kernel
