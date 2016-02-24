#ifndef MANTID_ALGORITHMS_SPECTRUMALGORITHM_H_
#define MANTID_ALGORITHMS_SPECTRUMALGORITHM_H_

#include <tuple>

#include "MantidKernel/SpectrumIndexSet.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidAlgorithms/Getters.h"

namespace Mantid {

namespace Kernel {
class SpectrumIndexSet;
}

namespace Algorithms {

class MANTID_ALGORITHMS_DLL SpectrumAlgorithm : public API::Algorithm {
private:
  // 3 little helpers for generating a sequence 1,2,3,.... Used for extracting
  // arguments from tuple.
  template <int...> struct seq {};
  template <int N, int... S> struct gens : gens<N - 1, N - 1, S...> {};
  template <int... S> struct gens<0, S...> { typedef seq<S...> type; };

  template <class WS, class T, int... S, class OP>
  void for_each(WS &workspace, T getters, seq<S...>, const OP &operation) {
    //auto count = static_cast<int64_t>(workspace.getNumberHistograms());

    auto indexSet = getSpectrumIndexSet(workspace);

    auto size = static_cast<int64_t>(indexSet.size());
    API::Progress progress(this, 0.0, 1.0, size);
    PARALLEL_FOR1((&workspace))
    for (int64_t i = 0; i < size; ++i) {
      PARALLEL_START_INTERUPT_REGION
      operation(std::get<S>(getters)(workspace, indexSet[i])...);
      progress.report(name());
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
  }

protected:
  template <class WS, class... Args, class OP>
  void for_each(WS &workspace, std::tuple<Args...> getters,
                const OP &operation) {
    for_each(workspace, getters, typename gens<sizeof...(Args)>::type(),
             operation);
  }
protected:
  void declareSpectrumIndexSetProperties();
  Kernel::SpectrumIndexSet
  getSpectrumIndexSet(const API::MatrixWorkspace &workspace) const;
};

/// Typedef for a shared pointer to a SpectrumAlgorithm
typedef boost::shared_ptr<SpectrumAlgorithm> SpectrumAlgorithm_sptr;

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_SPECTRUMALGORITHM_H_ */
