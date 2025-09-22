#include "MantidKernel/DllConfig.h"

#include <utility>
#include <vector>

#include "tbb/parallel_for.h"
#include "tbb/parallel_reduce.h"

namespace Mantid::Kernel {

/** MinMaxFinder
 * A templated functor for use in parallel_minmax, which will search a vector to find a min/max over a subrange.
 * These are then joined together for total min/max values
 */
template <typename T> class MANTID_KERNEL_DLL MinMaxFinder {
  std::vector<T> const *vec;

public:
  T minval;
  T maxval;
  void operator()(tbb::blocked_range<size_t> const &);

  // copy min/max from the other. we're all friends
  MinMaxFinder(MinMaxFinder<T> &other, tbb::split) : vec(other.vec), minval(other.minval), maxval(other.maxval) {}

  // set the min=max=first element supplied
  MinMaxFinder(std::vector<T> const *vec) : vec(vec), minval(vec->front()), maxval(vec->front()) {}

  void join(MinMaxFinder const &);
};

/** parallel_minmax
 * @param vec -- a pointer to a vector of values of type T, to search for a min and max
 * @param grainsize -- the grainsize for use in tbb::parallel_reduce.  Default = 1000
 * @return a std::pair<T,T> with the min (first) and max (second)
 */
template <typename T> std::pair<T, T> parallel_minmax(std::vector<T> const *const vec, size_t const grainsize = 1000);
} // namespace Mantid::Kernel
