// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Strings.h"
#include "MantidQtWidgets/Common/FunctionModelSpectra.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

#include <memory>
#include <optional>

#include <cctype>
#include <numeric>
#include <set>

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {
using namespace MantidWidgets;

/*
   FitData - Stores the data to be fit; workspace, spectra,
   fitting range and exclude regions. Provides methods for accessing
   and applying the fitting data.
*/
class MANTID_SPECTROSCOPY_DLL FitData {
public:
  FitData(const Mantid::API::MatrixWorkspace_sptr &workspace, const FunctionModelSpectra &spectra);

  std::string displayName(const std::string &formatString, const std::string &rangeDelimiter) const;
  std::string displayName(const std::string &formatString, WorkspaceIndex spectrum) const;
  std::string getBasename() const;

  Mantid::API::MatrixWorkspace_sptr workspace() const;
  const FunctionModelSpectra &spectra() const;
  FunctionModelSpectra &getMutableSpectra();
  WorkspaceIndex getSpectrum(FitDomainIndex index) const;
  FitDomainIndex numberOfSpectra() const;
  bool zeroSpectra() const;
  std::pair<double, double> getRange(WorkspaceIndex spectrum) const;
  std::string getExcludeRegion(WorkspaceIndex spectrum) const;
  FitData &combine(FitData const &fitData);

  std::vector<double> excludeRegionsVector(WorkspaceIndex spectrum) const;
  std::vector<double> getQValues() const;

  template <typename F> void applySpectra(F &&functor) const { ApplySpectra<F>(std::forward<F>(functor))(m_spectra); }

  template <typename F>
  WorkspaceIndex applyEnumeratedSpectra(F &&functor, WorkspaceIndex start = WorkspaceIndex{0}) const {
    return ApplyEnumeratedSpectra<F>(std::forward<F>(functor), start)(m_spectra);
  }

  void setSpectra(std::string const &spectra);
  void setSpectra(FunctionModelSpectra &&spectra);
  void setSpectra(FunctionModelSpectra const &spectra);
  void setStartX(double const &startX, WorkspaceIndex const &spectrum);
  void setStartX(double const &startX);
  void setEndX(double const &endX, WorkspaceIndex const &spectrum);
  void setEndX(double const &endX);
  void setExcludeRegionString(std::string const &excludeRegion, WorkspaceIndex const &spectrum);

private:
  void validateSpectra(FunctionModelSpectra const &spectra);

  Mantid::API::MatrixWorkspace_sptr m_workspace;
  FunctionModelSpectra m_spectra;
  std::map<WorkspaceIndex, std::string> m_excludeRegions;
  std::map<WorkspaceIndex, std::pair<double, double>> m_ranges;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
