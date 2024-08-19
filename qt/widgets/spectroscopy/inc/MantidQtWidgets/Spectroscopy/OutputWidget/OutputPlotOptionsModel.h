// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Plotting/ExternalPlotter.h"

#include "../DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <optional>

using namespace MantidQt::Widgets::MplCpp;

namespace MantidQt {
namespace CustomInterfaces {

class MANTID_SPECTROSCOPY_DLL IOutputPlotOptionsModel {
public:
  virtual ~IOutputPlotOptionsModel() = default;

  virtual bool setWorkspace(std::string const &workspaceName) = 0;
  virtual void removeWorkspace() = 0;
  virtual std::vector<std::string> getAllWorkspaceNames(std::vector<std::string> const &workspaceNames) const = 0;
  virtual std::optional<std::string> workspace() const = 0;
  virtual void setFixedIndices(std::string const &indices) = 0;
  virtual bool indicesFixed() const = 0;
  virtual void setUnit(std::string const &unit) = 0;
  virtual std::optional<std::string> unit() = 0;
  virtual std::string formatIndices(std::string const &indices) const = 0;
  virtual bool validateIndices(std::string const &indices, MantidAxis const &axisType = MantidAxis::Spectrum) const = 0;
  virtual bool setIndices(std::string const &indices) = 0;
  virtual std::optional<std::string> indices() const = 0;
  virtual void plotSpectra() = 0;
  virtual void plotBins(std::string const &binIndices) = 0;
  virtual void plotTiled() = 0;
  virtual void plot3DSurface() = 0;
  virtual void showSliceViewer() = 0;
  virtual std::optional<std::string> singleDataPoint(MantidAxis const &axisType) const = 0;
  virtual std::map<std::string, std::string> availableActions() const = 0;
};

class MANTID_SPECTROSCOPY_DLL OutputPlotOptionsModel final : public IOutputPlotOptionsModel {
public:
  OutputPlotOptionsModel(std::unique_ptr<IExternalPlotter> plotter,
                         std::optional<std::map<std::string, std::string>> const &availableActions = std::nullopt);
  virtual ~OutputPlotOptionsModel() override = default;

  bool setWorkspace(std::string const &workspaceName) override;
  void removeWorkspace() override;

  std::vector<std::string> getAllWorkspaceNames(std::vector<std::string> const &workspaceNames) const override;

  std::optional<std::string> workspace() const override;

  void setFixedIndices(std::string const &indices) override;
  bool indicesFixed() const override;

  void setUnit(std::string const &unit) override;
  std::optional<std::string> unit() override;

  std::string formatIndices(std::string const &indices) const override;
  bool validateIndices(std::string const &indices, MantidAxis const &axisType = MantidAxis::Spectrum) const override;
  bool setIndices(std::string const &indices) override;

  std::optional<std::string> indices() const override;

  void plotSpectra() override;
  void plotBins(std::string const &binIndices) override;
  void plotTiled() override;
  void plot3DSurface() override;
  void showSliceViewer() override;

  std::optional<std::string> singleDataPoint(MantidAxis const &axisType) const override;

  std::map<std::string, std::string> availableActions() const override;

private:
  bool validateSpectra(const Mantid::API::MatrixWorkspace_sptr &workspace, std::string const &spectra) const;
  bool validateBins(const Mantid::API::MatrixWorkspace_sptr &workspace, std::string const &bins) const;
  std::string convertUnit(const std::string &workspaceName, const std::string &unit);

  std::optional<std::string> checkWorkspaceSize(std::string const &workspaceName, MantidAxis const &axisType) const;

  std::map<std::string, std::string> m_actions;
  bool m_fixedIndices;
  std::optional<std::string> m_workspaceIndices;
  std::optional<std::string> m_workspaceName;
  std::optional<std::string> m_unit;
  std::unique_ptr<IExternalPlotter> m_plotter;
};

} // namespace CustomInterfaces
} // namespace MantidQt
