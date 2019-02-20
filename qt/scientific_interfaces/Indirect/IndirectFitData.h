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
#include "MantidKernel/Strings.h"

#include <boost/optional.hpp>
#include <boost/weak_ptr.hpp>

#include <cctype>
#include <numeric>

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
class Spectra {
public:
  explicit Spectra(const std::string &str)
      : m_vec(vectorFromString<size_t>(str)), m_isContinuous(true) {
    if (m_vec.size() > 1) {
      for (size_t i = 1; i < m_vec.size(); ++i) {
        if (m_vec[i] - m_vec[i - 1] != 1) {
          m_isContinuous = false;
          break;
        }
      }
    }
  }
  Spectra(size_t minimum, size_t maximum) : m_vec(maximum - minimum + 1) {
    std::iota(m_vec.begin(), m_vec.end(), minimum);
  }
  Spectra(const Spectra &vec)
      : m_vec(vec.m_vec), m_isContinuous(vec.m_isContinuous) {}
  Spectra(Spectra &&vec)
      : m_vec(std::move(vec.m_vec)), m_isContinuous(std::move(vec.m_isContinuous)) {}

  Spectra &operator=(const Spectra &vec) {
    m_vec = vec.m_vec;
    m_isContinuous = vec.m_isContinuous;
    return *this;
  }

  Spectra &operator=(Spectra &&vec) {
    m_vec = std::move(vec.m_vec);
    m_isContinuous = std::move(vec.m_isContinuous);
    return *this;
  }

  bool empty() const { return m_vec.empty(); }
  size_t size() const { return m_vec.size(); }
  std::string getString() const {
    if (empty()) return "";
    if (m_isContinuous) return size() > 1 ? std::to_string(m_vec.front()) + "-" + std::to_string(m_vec.back()) : std::to_string(m_vec.front());
    return Mantid::Kernel::Strings::toString(m_vec);
  }
  std::pair<size_t, size_t> getMinMax() const {
    if (empty()) return std::pair<size_t, size_t>(0, 0);
    return std::make_pair(m_vec.front(), m_vec.back());
  }
  std::vector<size_t>::iterator begin() { return m_vec.begin(); }
  std::vector<size_t>::iterator end() { return m_vec.end(); }
  std::vector<size_t>::const_iterator begin() const { return m_vec.begin(); }
  std::vector<size_t>::const_iterator end() const { return m_vec.end(); }
  const size_t &operator[](size_t index) const { return m_vec[index]; }
  bool operator==(Spectra const &spec) const {
    return this->getString() == spec.getString();
  }
  bool isContinuous() const { return m_isContinuous; }

private:
  std::vector<size_t> m_vec;
  bool m_isContinuous;
};

template <typename F> struct ApplySpectra {
  explicit ApplySpectra(F &&functor) : m_functor(std::forward<F>(functor)) {}

  void operator()(const Spectra &spectra) const {
    for (const auto &spectrum : spectra)
      m_functor(spectrum);
  }

private:
  F m_functor;
};

template <typename F>
struct ApplyEnumeratedSpectra {
  ApplyEnumeratedSpectra(F &&functor, size_t start = 0)
      : m_start(start), m_functor(std::forward<F>(functor)) {}

  size_t
  operator()(const Spectra &spectra) const {
    auto i = m_start;
    for (const auto &spectrum : spectra)
      m_functor(i++, spectrum);
    return i;
  }

private:
  size_t m_start;
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
                          size_t spectrum) const;
  std::string getBasename() const;

  Mantid::API::MatrixWorkspace_sptr workspace() const;
  const Spectra &spectra() const;
  size_t getSpectrum(size_t index) const;
  size_t numberOfSpectra() const;
  bool zeroSpectra() const;
  std::pair<double, double> getRange(size_t spectrum) const;
  std::string getExcludeRegion(size_t spectrum) const;
  IndirectFitData &combine(IndirectFitData const &fitData);

  std::vector<double> excludeRegionsVector(size_t spectrum) const;

  template <typename F> void applySpectra(F &&functor) const {
    ApplySpectra<F>(std::forward<F>(functor))(m_spectra);
  }

  template <typename F>
  size_t applyEnumeratedSpectra(F &&functor, size_t start = 0) const {
    return ApplyEnumeratedSpectra<F>(std::forward<F>(functor), start)(m_spectra);
  }

  void setSpectra(std::string const &spectra);
  void setSpectra(Spectra &&spectra);
  void setSpectra(Spectra const &spectra);
  void setStartX(double const &startX, size_t const &index);
  void setEndX(double const &endX, size_t const &spectrum);
  void setExcludeRegionString(std::string const &excludeRegion,
                              size_t const &spectrum);

private:
  void validateSpectra(Spectra const &spectra);

  Mantid::API::MatrixWorkspace_sptr m_workspace;
  Spectra m_spectra;
  std::unordered_map<size_t, std::string> m_excludeRegions;
  std::unordered_map<size_t, std::pair<double, double>> m_ranges;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
