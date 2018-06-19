#ifndef MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITDATA_H_
#define MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITDATA_H_

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ArrayProperty.h"

#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/weak_ptr.hpp>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

template <typename T>
std::vector<T> vectorFromString(const std::string &listString) {
  try {
    return Mantid::Kernel::ArrayProperty<T>("vector", listString);
  } catch (const std::runtime_error &) {
    return std::vector<T>();
  }
}

template <typename T> struct DiscontinuousSpectra {
public:
  DiscontinuousSpectra(const std::string &str)
      : m_str(str), m_vec(vectorFromString<T>(str)) {}
  DiscontinuousSpectra(const DiscontinuousSpectra &vec)
      : m_str(vec.m_str), m_vec(vec.m_vec) {}
  DiscontinuousSpectra(DiscontinuousSpectra &&vec)
      : m_str(std::move(vec.m_str)), m_vec(std::move(vec.m_vec)) {}

  DiscontinuousSpectra &operator=(const DiscontinuousSpectra &vec) {
    m_str = vec.m_str;
    m_vec = vec.m_vec;
    return *this;
  }

  DiscontinuousSpectra &operator=(DiscontinuousSpectra &&vec) {
    m_str = std::move(vec.m_str);
    m_vec = std::move(vec.m_vec);
    return *this;
  }

  const bool empty() const { return m_vec.empty(); }
  const std::size_t size() const { return m_vec.size(); }
  const std::string &getString() const { return m_str; }
  typename std::vector<T>::iterator begin() { return m_vec.begin(); }
  typename std::vector<T>::iterator end() { return m_vec.end(); }
  typename std::vector<T>::const_iterator begin() const {
    return m_vec.begin();
  }
  typename std::vector<T>::const_iterator end() const { return m_vec.end(); }
  const T &operator[](std::size_t index) const { return m_vec[index]; }

private:
  std::string m_str;
  std::vector<T> m_vec;
};

using Spectra = boost::variant<DiscontinuousSpectra<std::size_t>,
                               std::pair<std::size_t, std::size_t>>;

template <typename F> struct ApplySpectra : boost::static_visitor<> {
  ApplySpectra(F &&functor) : m_functor(functor) {}

  void operator()(const std::pair<std::size_t, std::size_t> &spectra) const {
    for (auto spectrum = spectra.first; spectrum <= spectra.second; ++spectrum)
      m_functor(spectrum);
  }

  void operator()(const DiscontinuousSpectra<std::size_t> &spectra) const {
    for (const auto &spectrum : spectra)
      m_functor(spectrum);
  }

private:
  F m_functor;
};

template <typename F>
struct ApplyEnumeratedSpectra : boost::static_visitor<std::size_t> {
  ApplyEnumeratedSpectra(F const &functor, std::size_t start = 0)
      : m_functor(functor), m_start(start) {}

  std::size_t
  operator()(const std::pair<std::size_t, std::size_t> &spectra) const {
    auto i = m_start;
    for (auto spectrum = spectra.first; spectrum <= spectra.second; ++spectrum)
      m_functor(i++, spectrum);
    return i;
  }

  std::size_t
  operator()(const DiscontinuousSpectra<std::size_t> &spectra) const {
    auto i = m_start;
    for (const auto &spectrum : spectra)
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
  std::string displayName(const std::string &formatString,
                          std::size_t spectrum) const;

  Mantid::API::MatrixWorkspace_sptr workspace() const;
  const Spectra &spectra() const;
  std::size_t getSpectrum(std::size_t index) const;
  std::size_t numberOfSpectra() const;
  bool zeroSpectra() const;
  std::pair<double, double> getRange(std::size_t spectrum) const;
  std::string getExcludeRegion(std::size_t spectrum) const;
  IndirectFitData &combine(const IndirectFitData &fitData);

  std::vector<double> excludeRegionsVector(std::size_t spectrum) const;

  template <typename F> void applySpectra(F &&functor) const {
    boost::apply_visitor(ApplySpectra<F>(functor), m_spectra);
  }

  template <typename F>
  std::size_t applyEnumeratedSpectra(F const &functor,
                                     std::size_t start = 0) const {
    return boost::apply_visitor(ApplyEnumeratedSpectra<F>(functor, start),
                                m_spectra);
  }

  void setSpectra(Spectra &&spectra);
  void setSpectra(const Spectra &spectra);
  void setStartX(double startX, std::size_t index);
  void setEndX(double endX, std::size_t spectrum);
  void setExcludeRegionString(const std::string &excludeRegion,
                              std::size_t spectrum);

private:
  void validateSpectra(const Spectra &spectra);

  boost::weak_ptr<Mantid::API::MatrixWorkspace> m_workspace;
  Spectra m_spectra;
  std::unordered_map<std::size_t, std::string> m_excludeRegions;
  std::unordered_map<std::size_t, std::pair<double, double>> m_ranges;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
