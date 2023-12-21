// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Plotting/ExternalPlotter.h"

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <boost/none_t.hpp>
#include <boost/optional.hpp>

using namespace MantidQt::Widgets::MplCpp;

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INELASTIC_DLL IndirectPlotOptionsModel {
public:
  IndirectPlotOptionsModel(boost::optional<std::map<std::string, std::string>> const &availableActions = boost::none);
  /// Used by the unit tests so that m_plotter can be mocked
  IndirectPlotOptionsModel(ExternalPlotter *plotter,
                           boost::optional<std::map<std::string, std::string>> const &availableActions = boost::none);
  virtual ~IndirectPlotOptionsModel();

  virtual bool setWorkspace(std::string const &workspaceName);
  virtual void removeWorkspace();

  virtual std::vector<std::string> getAllWorkspaceNames(std::vector<std::string> const &workspaceNames) const;

  boost::optional<std::string> workspace() const;

  virtual void setFixedIndices(std::string const &indices);
  virtual bool indicesFixed() const;

  virtual void setUnit(std::string const &unit);
  boost::optional<std::string> unit();

  virtual std::string formatIndices(std::string const &indices) const;
  virtual bool validateIndices(std::string const &indices, MantidAxis const &axisType = MantidAxis::Spectrum) const;
  virtual bool setIndices(std::string const &indices);

  boost::optional<std::string> indices() const;

  virtual void plotSpectra();
  virtual void plotBins(std::string const &binIndices);
  virtual void plotTiled();
  virtual void showSliceViewer();

  boost::optional<std::string> singleDataPoint(MantidAxis const &axisType) const;

  std::map<std::string, std::string> availableActions() const;

private:
  bool validateSpectra(const Mantid::API::MatrixWorkspace_sptr &workspace, std::string const &spectra) const;
  bool validateBins(const Mantid::API::MatrixWorkspace_sptr &workspace, std::string const &bins) const;
  std::string convertUnit(const std::string &workspaceName, const std::string &unit);

  boost::optional<std::string> checkWorkspaceSize(std::string const &workspaceName, MantidAxis const &axisType) const;

  std::map<std::string, std::string> m_actions;
  bool m_fixedIndices;
  boost::optional<std::string> m_workspaceIndices;
  boost::optional<std::string> m_workspaceName;
  boost::optional<std::string> m_unit;
  std::unique_ptr<ExternalPlotter> m_plotter;
};

} // namespace CustomInterfaces
} // namespace MantidQt
