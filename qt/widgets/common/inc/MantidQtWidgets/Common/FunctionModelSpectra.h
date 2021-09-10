// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "IndexTypes.h"
#include "MantidKernel/ArrayProperty.h"

#include <set>
#include <stdexcept>
#include <vector>

#include <QList>
#include <QString>
#include <QStringList>

namespace MantidQt {
namespace MantidWidgets {

/*
 * Representation of a discontinuous spectra range.
 * Can be used in a vector-like manner.
 *
 * Holds a string and vector representation.
 */
class EXPORT_OPT_MANTIDQT_COMMON FunctionModelSpectra {
public:
  explicit FunctionModelSpectra(const std::string &str);
  FunctionModelSpectra(WorkspaceIndex minimum, WorkspaceIndex maximum);
  FunctionModelSpectra(const FunctionModelSpectra &vec);
  FunctionModelSpectra(FunctionModelSpectra &&vec);
  FunctionModelSpectra &operator=(const FunctionModelSpectra &vec);
  FunctionModelSpectra &operator=(FunctionModelSpectra &&vec);
  bool empty() const;
  FitDomainIndex size() const;
  std::string getString() const;
  std::pair<WorkspaceIndex, WorkspaceIndex> getMinMax() const;
  WorkspaceIndex front() const { return m_vec.front(); }
  WorkspaceIndex back() const { return m_vec.back(); }
  std::vector<WorkspaceIndex>::const_iterator begin() const { return m_vec.cbegin(); }
  std::vector<WorkspaceIndex>::const_iterator end() const { return m_vec.cend(); }
  const WorkspaceIndex &operator[](FitDomainIndex index) const { return m_vec[index.value]; }
  bool operator==(FunctionModelSpectra const &spec) const;
  bool isContinuous() const;
  FitDomainIndex indexOf(WorkspaceIndex i) const;
  FunctionModelSpectra combine(const FunctionModelSpectra &other) const;
  void erase(WorkspaceIndex index);

private:
  explicit FunctionModelSpectra(const std::set<WorkspaceIndex> &indices);
  void checkContinuous();
  std::vector<WorkspaceIndex> m_vec;
  bool m_isContinuous;
};

template <typename F> struct ApplySpectra {
  explicit ApplySpectra(F &&functor) : m_functor(std::forward<F>(functor)) {}

  void operator()(const FunctionModelSpectra &spectra) const {
    for (const auto &spectrum : spectra)
      m_functor(spectrum);
  }

private:
  F m_functor;
};

template <typename F> struct ApplyEnumeratedSpectra {
  ApplyEnumeratedSpectra(F &&functor, WorkspaceIndex start = WorkspaceIndex{0})
      : m_start(start), m_functor(std::forward<F>(functor)) {}

  WorkspaceIndex operator()(const FunctionModelSpectra &spectra) const {
    auto i = m_start;
    for (const auto &spectrum : spectra)
      m_functor(i++, spectrum);
    return i;
  }

private:
  WorkspaceIndex m_start;
  F m_functor;
};

template <class T> std::vector<T> vectorFromString(const std::string &listString) {
  try {
    return Mantid::Kernel::ArrayProperty<T>("vector", listString);
  } catch (const std::runtime_error &) {
    return std::vector<T>();
  }
}

} // namespace MantidWidgets
} // namespace MantidQt
