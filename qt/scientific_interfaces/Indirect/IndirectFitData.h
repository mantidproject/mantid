// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITDATA_H_
#define MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITDATA_H_

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ArrayProperty.h"

#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/weak_ptr.hpp>

#include <cctype>

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

/*
 * Representation of a discontinuous spectra range.
 * Can be used in a vector-like manner.
 *
 * Holds a string and vector representation.
 */
template <typename T> class DiscontinuousSpectra {
public:
  explicit DiscontinuousSpectra(const std::string &str)
      : m_str(str), m_vec(vectorFromString<T>(str)) {
    m_str.erase(std::remove_if(m_str.begin(), m_str.end(),
                               static_cast<int (*)(int)>(std::isspace)),
                m_str.end());
  }
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

  bool empty() const { return m_vec.empty(); }
  std::size_t size() const { return m_vec.size(); }
  const std::string &getString() const { return m_str; }
  typename std::vector<T>::iterator begin() { return m_vec.begin(); }
  typename std::vector<T>::iterator end() { return m_vec.end(); }
  typename std::vector<T>::const_iterator begin() const {
    return m_vec.begin();
  }
  typename std::vector<T>::const_iterator end() const { return m_vec.end(); }
  const T &operator[](std::size_t index) const { return m_vec[index]; }
  bool operator==(DiscontinuousSpectra<std::size_t> const &spec) const {
    return this->getString() == spec.getString();
  }

private:
  std::string m_str;
  std::vector<T> m_vec;
};

/*
 * Spectra can either be specified as:
 *
 * Continuous Range - Represented as a pair of the minimum and maximum spectrum.
 * Discontinuous Range - Represented by a DiscontinuousSpectra object.
 *
 * A variant is used, such that faster operations can be employed when using
 * a continuous range.
 */
using Spectra = boost::variant<DiscontinuousSpectra<std::size_t>,
                               std::pair<std::size_t, std::size_t>>;

template <typename F> struct ApplySpectra : boost::static_visitor<> {
  explicit ApplySpectra(F &&functor) : m_functor(std::forward<F>(functor)) {}

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
  ApplyEnumeratedSpectra(F &&functor, std::size_t start = 0)
      : m_start(start), m_functor(std::forward<F>(functor)) {}

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

/*
   IndirectFitData - Stores the data to be fit; workspace, spectra,
   fitting range and exclude regions. Provides methods for accessing
   and applying the fitting data.
*/
class MANTIDQT_INDIRECT_DLL IndirectFitData {
public:
  IndirectFitData(Mantid::API::MatrixWorkspace_sptr workspace,
                  const Spectra &spectra);

  std::string displayName(const std::string &formatString,
                          const std::string &rangeDelimiter) const;
  std::string displayName(const std::string &formatString,
                          std::size_t spectrum) const;
  std::string getBasename() const;

  Mantid::API::MatrixWorkspace_sptr workspace() const;
  const Spectra &spectra() const;
  std::size_t getSpectrum(std::size_t index) const;
  std::size_t numberOfSpectra() const;
  bool zeroSpectra() const;
  std::pair<double, double> getRange(std::size_t spectrum) const;
  std::string getExcludeRegion(std::size_t spectrum) const;
  IndirectFitData &combine(IndirectFitData const &fitData);

  std::vector<double> excludeRegionsVector(std::size_t spectrum) const;

  template <typename F> void applySpectra(F &&functor) const {
    boost::apply_visitor(ApplySpectra<F>(std::forward<F>(functor)), m_spectra);
  }

  template <typename F>
  std::size_t applyEnumeratedSpectra(F &&functor, std::size_t start = 0) const {
    return boost::apply_visitor(
        ApplyEnumeratedSpectra<F>(std::forward<F>(functor), start), m_spectra);
  }

  void setSpectra(std::string const &spectra);
  void setSpectra(Spectra &&spectra);
  void setSpectra(Spectra const &spectra);
  void setStartX(double const &startX, std::size_t const &index);
  void setEndX(double const &endX, std::size_t const &spectrum);
  void setExcludeRegionString(std::string const &excludeRegion,
                              std::size_t const &spectrum);

private:
  void validateSpectra(Spectra const &spectra);

  Mantid::API::MatrixWorkspace_sptr m_workspace;
  Spectra m_spectra;
  std::unordered_map<std::size_t, std::string> m_excludeRegions;
  std::unordered_map<std::size_t, std::pair<double, double>> m_ranges;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
