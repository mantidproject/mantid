// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <tuple>

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidKernel/IndexSet.h"

namespace Mantid {

namespace DataObjects {
class EventWorkspace;
}

namespace Algorithms {

/** SpectrumAlgorithm is a base class for algorithms that work with
  MatrixWorkspace.

  This child class of Algorithm provides several features that make writing more
  generic and more compact code for algorithms easier. In particular it
  provides:
  1. The method for_each() that can be used to implement loops/transformations
     of spectra or event lists in a workspace.
  2. A way to define generic properties to allow user specified spectrum number
     ranges and list.

  @author Simon Heybrock, ESS
*/
class MANTID_ALGORITHMS_DLL SpectrumAlgorithm : public API::Algorithm {
private:
  /** Helpers for for_each(), struct seq and gens with a specialization.
   *
   * Used for generating a sequence 0,1,2,.... for extracting arguments from
   * tuple. */
  template <std::size_t...> struct seq {};
  // Doxygen does not like recursive types.
  template <std::size_t N, std::size_t... S>
  struct gens                                      /** @cond */
      : gens<N - 1, N - 1, S...> /** @endcond */ { /** @cond */
  };
  template <std::size_t... S> struct gens<0, S...> { /** @endcond */
    using type = seq<S...>;
  };

  /** Helpers for for_each(), struct contains and 2 specializations.
   *
   * Used to determine at compile time if parameter pack contains a certain
   * type. */
  template <typename Tp, typename... List> struct contains : std::true_type {};
  template <typename Tp, typename Head, typename... Rest>
  struct contains<Tp, Head, Rest...>
      : std::conditional<std::is_same<Tp, Head>::value, std::true_type, contains<Tp, Rest...>>::type {};
  template <typename Tp> struct contains<Tp> : std::false_type {};

  /// Internal implementation of for_each().
  template <class... Flags, class WS, class T, std::size_t... S, class OP>
  void for_each(WS &workspace, T getters, seq<S...>, const OP &operation) {
    // If we get the flag Indices::FromProperty we use a potential user-defined
    // range property, otherwise default to the full range of all histograms.
    Kernel::IndexSet indexSet(workspace.getNumberHistograms());
    if (contains<Indices::FromProperty, Flags...>())
      indexSet = getWorkspaceIndexSet(workspace);
    auto size = static_cast<int64_t>(indexSet.size());
    API::Progress progress(this, 0.0, 1.0, size);

    PARALLEL_FOR_IF(Kernel::threadSafe(workspace))
    for (int64_t i = 0; i < size; ++i) {
      PARALLEL_START_INTERRUPT_REGION
      // Note the small but for now negligible overhead from the IndexSet access
      // in the case where it is not used.
      operation(std::get<S>(getters)(workspace, indexSet[i])...);
      progress.report(name());
      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION

    ifEventWorkspaceClearMRU(workspace);
  }

  /// Templated function used as 'compile-time conditional', together with
  /// specialization. See there for details.
  template <class WS> void ifEventWorkspaceClearMRU(const WS &workspace) { UNUSED_ARG(workspace); }

  std::string m_indexMinPropertyName;
  std::string m_indexMaxPropertyName;
  std::string m_indexRangePropertyName;

protected:
  ~SpectrumAlgorithm() override = default;

  /// Dummy struct holding compile-time flags to for_each().
  // A strongly typed enum used in non-type variadic template arguments would
  // have been the other candidate. However, combining flags from multiple enums
  // would not have been possible, so all flags would have been required to be
  // in the same scoped enum, and we want to avoid noisy flag definitions like
  // Flags::IndicesFromProperty or Flags::SkipMasked.
  struct Indices {
    /// Flag: Include only indices specified via properties in for_each.
    struct FromProperty {};
  };

  /** Provides a mechanism for looping over spectra in a workspace.
   *
   * This variant works with a single workspace and can to in-place modification
   * of spectra or event lists in the workspace. Threading and progress
   * reporting is handled internally.
   * @tparam Flags Variable number of flags, see struct Indices.
   * @param workspace Workspace to work with.
   * @param getters Tuple of getters, pointers to methods of workspace, defaults
   *     defined for convenience in namespace Getters.
   * @param operation Callable that is executed for all spectra (etc.). Results
   *     from getters are passed as arguments. */
  template <class... Flags, class WS, class... Args, class OP>
  void for_each(WS &workspace, std::tuple<Args...> getters, const OP &operation) {
    // This comment explains some of the rationale and mechanism for the way
    // templates are used in this and other variants of for_each().
    // For several variants of for_each() we require a variable number of
    // arguments for more than one entity, e.g., getters for the input workspace
    // and getters for the output workspace. For both of them we would thus like
    // to use a variadic template. However, we cannot make all those getters
    // direct arguments to for_each(), because the compiler then cannot tell
    // were the boundary between the two parameter packs is. Therefore getters
    // are packed into tuples.
    // At some point we need to extract the getters from the tuple. To this end,
    // we use the gens template, which recursively constructs a sequence of
    // indices which are gen used with std::get<>.
    // The Flags template parameter is used for passing in flags known at
    // compile time.
    for_each<Flags...>(workspace, getters, typename gens<sizeof...(Args)>::type(), operation);
  }

  void declareWorkspaceIndexSetProperties(const std::string &indexMinPropertyName = "IndexMin",
                                          const std::string &indexMaxPropertyName = "IndexMax",
                                          const std::string &indexRangePropertyName = "WorkspaceIndexList");
  Kernel::IndexSet getWorkspaceIndexSet(const API::MatrixWorkspace &workspace) const;
};

template <> void SpectrumAlgorithm::ifEventWorkspaceClearMRU(const DataObjects::EventWorkspace &workspace);

/// Typedef for a shared pointer to a SpectrumAlgorithm
using SpectrumAlgorithm_sptr = std::shared_ptr<SpectrumAlgorithm>;

} // namespace Algorithms
} // namespace Mantid
