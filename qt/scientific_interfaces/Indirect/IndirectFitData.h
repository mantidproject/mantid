#ifndef MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITDATA_H_
#define MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITDATA_H_

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ArrayProperty.h"

#include <boost/variant.hpp>
#include <boost/weak_ptr.hpp>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

enum class FittingMode { SEQUENTIAL, SIMULTANEOUS };

using Spectra =
    boost::variant<std::string, std::pair<std::size_t, std::size_t>>;

template <typename T>
std::vector<T> vectorFromString(const std::string &listString) {
  try {
    return Mantid::Kernel::ArrayProperty<T>("vector", listString);
  } catch (const std::runtime_error &) {
    return std::vector<T>();
  }
}

template <typename F> struct ApplySpectra : boost::static_visitor<> {
  ApplySpectra(F &&functor) : m_functor(functor) {}

  void operator()(const std::pair<std::size_t, std::size_t> &spectra) const {
    for (auto spectrum = spectra.first; spectrum < spectra.second; ++spectrum)
      m_functor(spectrum);
  }

  void operator()(const std::string &spectra) const {
    for (const auto &spectrum : vectorFromString<std::size_t>(spectra))
      m_functor(spectrum);
  }

private:
  F m_functor;
};

template <typename F>
struct ApplyEnumeratedSpectra : boost::static_visitor<std::size_t> {
  ApplyEnumeratedSpectra(F &&functor, std::size_t start = 0)
      : m_functor(functor), m_start(start) {}

  std::size_t
  operator()(const std::pair<std::size_t, std::size_t> &spectra) const {
    auto i = m_start;
    for (auto spectrum = spectra.first; spectrum < spectra.second; ++spectrum)
      m_functor(++i, spectrum);
    return i;
  }

  std::size_t operator()(const std::string &spectra) const {
    const auto spectraVector = vectorFromString<std::size_t>(spectra);
    auto i = m_start;
    for (const auto &spectrum : spectraVector)
      m_functor(i++, spectrum);
    return i;
  }

private:
  std::size_t m_start;
  F m_functor;
};

class IndirectFitData {
public:
  IndirectFitData(Mantid::API::MatrixWorkspace_sptr workspace,
                  const Spectra &spectra);

  std::string displayName(const std::string &formatString,
                          const std::string &rangeDelimiter) const;
  Mantid::API::MatrixWorkspace_sptr workspace() const;
  const Spectra &spectra() const;
  std::size_t getSpectrum(std::size_t index) const;
  std::size_t numberOfSpectra() const;
  bool zeroSpectra() const;
  const std::pair<double, double> &range(std::size_t spectrum) const;
  std::size_t firstSpectrum() const;
  std::string excludeRegionString(std::size_t spectrum) const;
  IndirectFitData &combine(const IndirectFitData &fitData);

  std::vector<double> excludeRegionsVector(std::size_t spectrum) const;

  template <typename F> void applySpectra(F &&functor) const {
    return boost::apply_visitor(ApplySpectra<F>(functor), m_spectra);
  }

  template <typename F>
  void applyEnumeratedSpectra(F &&functor, std::size_t start = 0) const {
    return boost::apply_visitor(ApplyEnumeratedSpectra<F>(functor, start),
                                m_spectra);
  }

  void setSpectra(const Spectra &spectra);
  void setExcludeRegionString(std::size_t spectrum,
                              const std::string &excludeRegion);

private:
  boost::weak_ptr<Mantid::API::MatrixWorkspace> m_workspace;
  Spectra m_spectra;
  std::unordered_map<std::size_t, std::string> m_excludeRegions;
  std::unordered_map<std::size_t, std::pair<double, double>> m_ranges;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
