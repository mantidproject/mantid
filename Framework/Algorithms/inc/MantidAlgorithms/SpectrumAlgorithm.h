#ifndef MANTID_ALGORITHMS_SPECTRUMALGORITHM_H_
#define MANTID_ALGORITHMS_SPECTRUMALGORITHM_H_

#include <tuple>

#include "MantidKernel/IndexSet.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {

namespace DataObjects {
class EventWorkspace;
}

namespace Algorithms {

class MANTID_ALGORITHMS_DLL SpectrumAlgorithm : public API::Algorithm {
private:
  // 3 little helpers for generating a sequence 1,2,3,.... Used for extracting
  // arguments from tuple.
  template <int...> struct seq {};
  template <int N, int... S> struct gens : gens<N - 1, N - 1, S...> {};
  template <int... S> struct gens<0, S...> { typedef seq<S...> type; };

  /// Helper for for_each (struct contains and 2 specializations). Used to
  /// determine at compile time if parameter pack contains a certain type.
  template <typename Tp, typename... List> struct contains : std::true_type {};
  template <typename Tp, typename Head, typename... Rest>
  struct contains<Tp, Head, Rest...>
      : std::conditional<std::is_same<Tp, Head>::value, std::true_type,
                         contains<Tp, Rest...>>::type {};
  template <typename Tp> struct contains<Tp> : std::false_type {};

  template <class... Flags, class WS, class T, int... S, class OP>
  void for_each(WS &workspace, T getters, seq<S...>, const OP &operation) {
    // If we get the flag Indices::FromProperty we use a potential user-defined
    // range property, otherwise default to the full range of all histograms.
    Kernel::IndexSet indexSet(workspace.getNumberHistograms());
    if (contains<Indices::FromProperty, Flags...>())
      indexSet = getSpectrumIndexSet(workspace);
    auto size = static_cast<int64_t>(indexSet.size());
    API::Progress progress(this, 0.0, 1.0, size);

    PARALLEL_FOR1((&workspace))
    for (int64_t i = 0; i < size; ++i) {
      PARALLEL_START_INTERUPT_REGION
      // Note the small but for now negligible overhead from the IndexSet access
      // in the case where it is not used.
      operation(std::get<S>(getters)(workspace, indexSet[i])...);
      progress.report(name());
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

    ifEventWorkspaceClearMRU(workspace);
  }

  template <class WS> void ifEventWorkspaceClearMRU(const WS &workspace) {
    UNUSED_ARG(workspace);
  }

protected:
  /// These structs are used as compile-time flags to for_each. A strongly typed
  /// enum used in non-type variadic template arguments would have been the
  /// other candidate. However, combining flags from multiple enums would not
  /// have been possible, so all flags would have been required to be in the
  /// same scoped enum, and we want to avoid noisy flag definitions like
  /// Flags::IndicesFromProperty or Flags::SkipMasked.
  struct Indices {
    struct FromProperty {};
  };

  template <class... Flags, class WS, class... Args, class OP>
  void for_each(WS &workspace, std::tuple<Args...> getters,
                const OP &operation) {
    for_each<Flags...>(workspace, getters,
                       typename gens<sizeof...(Args)>::type(), operation);
  }

  void declareSpectrumIndexSetProperties();
  Kernel::IndexSet
  getSpectrumIndexSet(const API::MatrixWorkspace &workspace) const;
};

template <>
void SpectrumAlgorithm::ifEventWorkspaceClearMRU(
    const DataObjects::EventWorkspace &workspace);

/// Typedef for a shared pointer to a SpectrumAlgorithm
typedef boost::shared_ptr<SpectrumAlgorithm> SpectrumAlgorithm_sptr;

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_SPECTRUMALGORITHM_H_ */
