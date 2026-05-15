// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "GUI/Common/PlotOptionsProvider.h"
#include "GUI/Common/Plotter.h"
#include "IPlottingModel.h"
#include "IPlottingPresenter.h"
#include "IPlottingView.h"
#include "PlottingModel.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL PlottingPresenter : public IPlottingPresenter, public PlottingViewSubscriber {
public:
  explicit PlottingPresenter(IPlottingView *view);
  PlottingPresenter(IPlottingView *view, IPlotter const &plotter, IPlotOptionsProvider const &plotOptionsProvider,
                    IPlottingModel const &plottingModel);

  void acceptMainPresenter(IBatchPresenter *mainPresenter) override;
  void notifyReductionPaused() override;
  void notifyReductionResumed() override;
  void notifyAutoreductionPaused() override;
  void notifyAutoreductionResumed() override;
  void notifyInstrumentChanged(std::string const &instrumentName) override;
  void notifyRunsTableChanged(RunsTable const &runsTable) override;
  void notifyPlotTiledClicked() override;
  void notifyPlotOverplotClicked() override;
  void notifyPlotIndividualClicked() override;

private:
  std::vector<PlottingWorkspaceTreeItem> makeWorkspaceItems(RunsTable const &runsTable) const;
  void plotSelectedWorkspaces(PlotLayout layout) const;
  void updateAvailablePlotOutputTypes(std::string const &instrumentName);
  void updateWidgetEnabledState();
  bool isProcessing() const;
  bool isAutoreducing() const;

  Plotter m_defaultPlotter;
  PlotOptionsProvider m_defaultPlotOptionsProvider;
  PlottingModel m_defaultPlottingModel;
  IPlottingView *m_view;
  IBatchPresenter *m_mainPresenter;
  IPlotter const *m_plotter;
  IPlotOptionsProvider const *m_plotOptionsProvider;
  IPlottingModel const *m_plottingModel;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
