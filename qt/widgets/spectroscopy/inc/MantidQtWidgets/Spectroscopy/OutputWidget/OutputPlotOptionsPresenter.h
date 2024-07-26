// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Spectroscopy/InelasticInterface.h"
#include "MantidQtWidgets/Spectroscopy/InelasticTab.h"
#include "MantidQtWidgets/Spectroscopy/OutputWidget/OutputPlotOptionsModel.h"
#include "MantidQtWidgets/Spectroscopy/OutputWidget/OutputPlotOptionsView.h"

#include "MantidAPI/AnalysisDataServiceObserver.h"

#include "../DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTID_SPECTROSCOPY_DLL IOutputPlotOptionsPresenter {
public:
  virtual void handleWorkspaceChanged(std::string const &workspaceName) = 0;
  virtual void handleSelectedUnitChanged(std::string const &unit) = 0;
  virtual void handleSelectedIndicesChanged(std::string const &indices) = 0;
  virtual void handlePlotSpectraClicked() = 0;
  virtual void handlePlotBinsClicked() = 0;
  virtual void handleShowSliceViewerClicked() = 0;
  virtual void handlePlotTiledClicked() = 0;
  virtual void handlePlot3DClicked() = 0;
};

class MANTID_SPECTROSCOPY_DLL OutputPlotOptionsPresenter final : public IOutputPlotOptionsPresenter,
                                                                 public AnalysisDataServiceObserver {

public:
  OutputPlotOptionsPresenter(IOutputPlotOptionsView *view, PlotWidget const &plotType = PlotWidget::Spectra,
                             std::string const &fixedIndices = "",
                             std::optional<std::map<std::string, std::string>> const &availableActions = std::nullopt);
  /// Used by the unit tests so that the view and model can be mocked
  OutputPlotOptionsPresenter(IOutputPlotOptionsView *view, std::unique_ptr<IOutputPlotOptionsModel> model,
                             PlotWidget const &plotType = PlotWidget::Spectra, std::string const &fixedIndices = "");
  ~OutputPlotOptionsPresenter() = default;

  void handleWorkspaceChanged(std::string const &workspaceName) override;
  void handleSelectedUnitChanged(std::string const &unit) override;
  void handleSelectedIndicesChanged(std::string const &indices) override;
  void handlePlotSpectraClicked() override;
  void handlePlotBinsClicked() override;
  void handleShowSliceViewerClicked() override;
  void handlePlot3DClicked() override;
  void handlePlotTiledClicked() override;

  void setPlotType(PlotWidget const &plotType);

  void setWorkspaces(std::vector<std::string> const &workspaces);
  void clearWorkspaces();

private:
  void setupPresenter(PlotWidget const &plotType, std::string const &fixedIndices);
  void watchADS(bool on);

  void setPlotting(bool plotting);
  void setOptionsEnabled(bool enable);

  void replaceHandle(const std::string &wsName, const Workspace_sptr &workspace) override;
  void deleteHandle(const std::string &wsName, const Workspace_sptr &workspace) override;

  void setWorkspace(std::string const &plotWorkspace);
  void setUnit(std::string const &unit);
  void setIndices();

  bool validateWorkspaceSize(MantidAxis const &axisType);

  IOutputPlotOptionsView *m_view;
  std::unique_ptr<IOutputPlotOptionsModel> m_model;
  PlotWidget m_plotType;
};

} // namespace CustomInterfaces
} // namespace MantidQt
