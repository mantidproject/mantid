// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITDATA_H_
#define MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITDATA_H_

#include "DllConfig.h"
#include "IndexTypes.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Strings.h"

#include <boost/optional.hpp>
#include <boost/weak_ptr.hpp>

#include <cctype>
#include <numeric>
#include <set>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

/*
 * Representation of a discontinuous spectra range.
 * Can be used in a vector-like manner.
 *
 * Holds a string and vector representation.
 */
class MANTIDQT_INDIRECT_DLL Spectra {
public:
  explicit Spectra(const std::string &str);
  Spectra(WorkspaceIndex minimum, WorkspaceIndex maximum);
  Spectra(const Spectra &vec);
  Spectra(Spectra &&vec);
  Spectra &operator=(const Spectra &vec);
  Spectra &operator=(Spectra &&vec);
  bool empty() const;
  SpectrumRowIndex size() const;
  std::string getString() const;
  std::pair<WorkspaceIndex, WorkspaceIndex> getMinMax() const;
  WorkspaceIndex front() const { return m_vec.front(); }
  WorkspaceIndex back() const { return m_vec.back(); }
  std::vector<WorkspaceIndex>::iterator begin() { return m_vec.begin(); }
  std::vector<WorkspaceIndex>::iterator end() { return m_vec.end(); }
  std::vector<WorkspaceIndex>::const_iterator begin() const {
    return m_vec.begin();
  }
  std::vector<WorkspaceIndex>::const_iterator end() const {
    return m_vec.end();
  }
  const WorkspaceIndex &operator[](SpectrumRowIndex index) const {
    return m_vec[index.value];
  }
  bool operator==(Spectra const &spec) const;
  bool isContinuous() const;
  SpectrumRowIndex indexOf(WorkspaceIndex i) const;
  Spectra combine(const Spectra &other) const;

private:
  explicit Spectra(const std::set<WorkspaceIndex> &indices);
  void checkContinuous();
  std::vector<WorkspaceIndex> m_vec;
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

template <typename F> struct ApplyEnumeratedSpectra {
  ApplyEnumeratedSpectra(F &&functor, WorkspaceIndex start = WorkspaceIndex{0})
      : m_start(start), m_functor(std::forward<F>(functor)) {}

  WorkspaceIndex operator()(const Spectra &spectra) const {
    auto i = m_start;
    for (const auto &spectrum : spectra)
      m_functor(i++, spectrum);
    return i;
  }

private:
  WorkspaceIndex m_start;
  F m_functor;
};

template <class T>
std::vector<T> vectorFromString(const std::string &listString) {
  try {
    return Mantid::Kernel::ArrayProperty<T>("vector", listString);
  } catch (const std::runtime_error &) {
    return std::vector<T>();
  }
}

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
                          WorkspaceIndex spectrum) const;
  std::string getBasename() const;

  Mantid::API::MatrixWorkspace_sptr workspace() const;
  const Spectra &spectra() const;
  WorkspaceIndex getSpectrum(SpectrumRowIndex index) const;
  SpectrumRowIndex numberOfSpectra() const;
  bool zeroSpectra() const;
  std::pair<double, double> getRange(WorkspaceIndex spectrum) const;
  std::string getExcludeRegion(WorkspaceIndex spectrum) const;
  IndirectFitData &combine(IndirectFitData const &fitData);

  std::vector<double> excludeRegionsVector(WorkspaceIndex spectrum) const;

  template <typename F> void applySpectra(F &&functor) const {
    ApplySpectra<F>(std::forward<F>(functor))(m_spectra);
  }

  template <typename F>
  WorkspaceIndex
  applyEnumeratedSpectra(F &&functor,
                         WorkspaceIndex start = WorkspaceIndex{0}) const {
    return ApplyEnumeratedSpectra<F>(std::forward<F>(functor),
                                     start)(m_spectra);
  }

  void setSpectra(std::string const &spectra);
  void setSpectra(Spectra &&spectra);
  void setSpectra(Spectra const &spectra);
  void setStartX(double const &startX, WorkspaceIndex const &index);
  void setStartX(double const &startX);
  void setEndX(double const &endX, WorkspaceIndex const &spectrum);
  void setEndX(double const &endX);
  void setExcludeRegionString(std::string const &excludeRegion,
                              WorkspaceIndex const &spectrum);

private:
  void validateSpectra(Spectra const &spectra);

  Mantid::API::MatrixWorkspace_sptr m_workspace;
  Spectra m_spectra;
  std::map<WorkspaceIndex, std::string> m_excludeRegions;
  std::map<WorkspaceIndex, std::pair<double, double>> m_ranges;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
