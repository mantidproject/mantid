#ifndef MANTID_ALGORITHMS_SPECTRUMALGORITHM_H_
#define MANTID_ALGORITHMS_SPECTRUMALGORITHM_H_

#include <tuple>

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/Getters.h"
#include "MantidAlgorithms/GeometryInfo.h"

namespace Mantid {
namespace Algorithms {

class MANTID_ALGORITHMS_DLL SpectrumAlgorithm : public API::Algorithm {
public:
  virtual ~SpectrumAlgorithm() override = default;

private:
  // 3 little helpers for generating a sequence 1,2,3,.... Used for extracting
  // arguments from tuple.
  template <int...> struct seq {};
  template <int N, int... S> struct gens : gens<N - 1, N - 1, S...> {};
  template <int... S> struct gens<0, S...> { typedef seq<S...> type; };

  template <class WS, class T1, int... S1, class T2, int... S2, class T3,
            int... S3, class OP>
  void for_each(const WS &inputWorkspace, T1 inputGetters, seq<S1...>,
                T2 geometryGetters, seq<S2...>, WS &outputWorkspace,
                T3 outputGetters, seq<S3...>, const OP &operation) {
    auto count = static_cast<int64_t>(inputWorkspace.getNumberHistograms());
    API::Progress progress(this, 0.0, 1.0, count);
    BasicInstrumentInfo info(inputWorkspace);

    PARALLEL_FOR2((&inputWorkspace), (&outputWorkspace))
    for (int i = 0; i < count; ++i) {
      PARALLEL_START_INTERUPT_REGION
      try {
        GeometryInfo geom(info, *(inputWorkspace.getSpectrum(i)));
        operation(std::get<S1>(inputGetters)(inputWorkspace, i)...,
                  std::get<S2>(geometryGetters)(geom)...,
                  std::get<S3>(outputGetters)(outputWorkspace, i)...);
      } catch (Kernel::Exception::NotFoundError &) {
        g_log.warning() << "Spectrum index " << i
                        << " has no detector assigned to it -- skipping"
                        << std::endl;
      }
      progress.report(name());
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
  }

protected:
  template <class WS, class... Args1, class... Args2, class... Args3, class OP>
  void for_each(const WS &inputWorkspace, std::tuple<Args1...> inputGetters,
                std::tuple<Args2...> geometryGetters, WS &outputWorkspace,
                std::tuple<Args3...> outputGetters, const OP &operation) {
    for_each(inputWorkspace, inputGetters,
             typename gens<sizeof...(Args1)>::type(), geometryGetters,
             typename gens<sizeof...(Args2)>::type(), outputWorkspace,
             outputGetters, typename gens<sizeof...(Args3)>::type(), operation);
  }
};

/// Typedef for a shared pointer to a SpectrumAlgorithm
typedef boost::shared_ptr<SpectrumAlgorithm> SpectrumAlgorithm_sptr;

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SPECTRUMALGORITHM_H_*/
