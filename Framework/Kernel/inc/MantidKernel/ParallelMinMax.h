#include "MantidKernel/DllConfig.h"

#include <utility>
#include <vector>

#include "tbb/parallel_for.h"
#include "tbb/parallel_reduce.h"

namespace Mantid::Kernel {

/** parallel_minmax
 * @param vec -- a pointer to a vector of values of type T, to search for a min and max
 * @param grainsize -- the grainsize for use in tbb::parallel_reduce.  Default = 1000
 * @return a std::pair<T,T> with the min (first) and max (second)
 */
template <typename T> std::pair<T, T> parallel_minmax(std::vector<T> const *const vec, size_t const grainsize = 1000);

/** parallel_minmax
 * @param vec -- a shared pointer to a vector of values of type T, to search for a min and max
 * @param grainsize -- the grainsize for use in tbb::parallel_reduce.  Default = 1000
 * @return a std::pair<T,T> with the min (first) and max (second)
 */
template <typename T>
std::pair<T, T> parallel_minmax(std::shared_ptr<std::vector<T>> const &vec, size_t const grainsize = 1000);

/** parallel_minmax
 * @param vec -- a unique pointer to a vector of values of type T, to search for a min and max
 * @param grainsize -- the grainsize for use in tbb::parallel_reduce.  Default = 1000
 * @return a std::pair<T,T> with the min (first) and max (second)
 */
template <typename T>
std::pair<T, T> parallel_minmax(std::unique_ptr<std::vector<T>> const &vec, size_t const grainsize = 1000);

} // namespace Mantid::Kernel
