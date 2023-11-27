// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectInterface.h"
#include "IndirectPlotOptionsModel.h"
#include "IndirectPlotOptionsView.h"
#include "IndirectTab.h"

#include "DllConfig.h"

#include <Poco/NObserver.h>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INDIRECT_DLL IIndirectPlotOptionsPresenter {
public:
  virtual void handleWorkspaceChanged(std::string const &workspaceName) = 0;
  virtual void handleSelectedUnitChanged(std::string const &unit) = 0;
  virtual void handleSelectedIndicesChanged(std::string const &indices) = 0;
  virtual void handlePlotSpectraClicked() = 0;
  virtual void handlePlotBinsClicked() = 0;
  virtual void handleShowSliceViewerClicked() = 0;
  virtual void handlePlotTiledClicked() = 0;
};

class MANTIDQT_INDIRECT_DLL IndirectPlotOptionsPresenter final : public IIndirectPlotOptionsPresenter {

public:
  IndirectPlotOptionsPresenter(
      IIndirectPlotOptionsView *view, PlotWidget const &plotType = PlotWidget::Spectra,
      std::string const &fixedIndices = "",
      boost::optional<std::map<std::string, std::string>> const &availableActions = boost::none);
  /// Used by the unit tests so that the view and model can be mocked
  IndirectPlotOptionsPresenter(IIndirectPlotOptionsView *view, IndirectPlotOptionsModel *model,
                               PlotWidget const &plotType = PlotWidget::Spectra, std::string const &fixedIndices = "");
  ~IndirectPlotOptionsPresenter();

  void handleWorkspaceChanged(std::string const &workspaceName) override;
  void handleSelectedUnitChanged(std::string const &unit) override;
  void handleSelectedIndicesChanged(std::string const &indices) override;
  void handlePlotSpectraClicked() override;
  void handlePlotBinsClicked() override;
  void handleShowSliceViewerClicked() override;
  void handlePlotTiledClicked() override;

  void setPlotType(PlotWidget const &plotType);

  void setWorkspaces(std::vector<std::string> const &workspaces);
  void clearWorkspaces();

private:
  void setupPresenter(PlotWidget const &plotType, std::string const &fixedIndices);
  void watchADS(bool on);

  void setPlotting(bool plotting);
  void setOptionsEnabled(bool enable);

  void onWorkspaceRemoved(Mantid::API::WorkspacePreDeleteNotification_ptr nf);
  void onWorkspaceReplaced(Mantid::API::WorkspaceBeforeReplaceNotification_ptr nf);

  void setWorkspace(std::string const &plotWorkspace);
  void setUnit(std::string const &unit);
  void setIndices();

  bool validateWorkspaceSize(MantidAxis const &axisType);

  // Observers for ADS Notifications
  Poco::NObserver<IndirectPlotOptionsPresenter, Mantid::API::WorkspacePreDeleteNotification> m_wsRemovedObserver;
  Poco::NObserver<IndirectPlotOptionsPresenter, Mantid::API::WorkspaceBeforeReplaceNotification> m_wsReplacedObserver;

  IIndirectPlotOptionsView *m_view;
  std::unique_ptr<IndirectPlotOptionsModel> m_model;
  PlotWidget m_plotType;
};

} // namespace CustomInterfaces
} // namespace MantidQt
