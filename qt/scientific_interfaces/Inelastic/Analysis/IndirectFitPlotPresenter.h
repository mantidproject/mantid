// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectFitPlotModel.h"
#include "IndirectFitPlotView.h"

#include "DllConfig.h"
#include "MantidQtWidgets/Plotting/ExternalPlotter.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
using namespace MantidWidgets;

class MANTIDQT_INELASTIC_DLL IIndirectFitPlotPresenter {
public:
  virtual void handleSelectedFitDataChanged(WorkspaceID workspaceID) = 0;
  virtual void handlePlotSpectrumChanged(WorkspaceIndex spectrum) = 0;
  virtual void handlePlotCurrentPreview() = 0;
  virtual void handlePlotGuess(bool doPlotGuess) = 0;
  virtual void handleFitSingleSpectrum() = 0;

  virtual void handleStartXChanged(double value) = 0;
  virtual void handleEndXChanged(double value) = 0;

  virtual void handleHWHMMinimumChanged(double maximum) = 0;
  virtual void handleHWHMMaximumChanged(double minimum) = 0;

  virtual void handleFWHMChanged(double minimum, double maximum) = 0;
  virtual void handleBackgroundChanged(double value) = 0;
};

class MANTIDQT_INELASTIC_DLL IndirectFitPlotPresenter : public QObject, public IIndirectFitPlotPresenter {
  Q_OBJECT

public:
  IndirectFitPlotPresenter(IIndirectFitPlotView *view, std::unique_ptr<IndirectFitPlotModel> model);

  void watchADS(bool watch);

  WorkspaceID getActiveWorkspaceID() const;
  WorkspaceIndex getActiveWorkspaceIndex() const;
  FitDomainIndex getSelectedDomainIndex() const;
  bool isCurrentlySelected(WorkspaceID workspaceID, WorkspaceIndex spectrum) const;

  void setFittingData(std::vector<IndirectFitData> *fittingData);
  void setFitOutput(IIndirectFitOutput *fitOutput);
  void setFitFunction(Mantid::API::MultiDomainFunction_sptr function);
  void setFitSingleSpectrumIsFitting(bool fitting);
  void setFitSingleSpectrumEnabled(bool enable);

  void setXBounds(std::pair<double, double> const &bounds);

  void handleSelectedFitDataChanged(WorkspaceID workspaceID) override;
  void handlePlotSpectrumChanged(WorkspaceIndex spectrum) override;
  void handlePlotCurrentPreview() override;
  void handlePlotGuess(bool doPlotGuess) override;
  void handleFitSingleSpectrum() override;

  void handleStartXChanged(double value) override;
  void handleEndXChanged(double value) override;

  void handleHWHMMinimumChanged(double maximum) override;
  void handleHWHMMaximumChanged(double minimum) override;

  void handleFWHMChanged(double minimum, double maximum) override;
  void handleBackgroundChanged(double value) override;

public slots:
  void setStartX(double /*startX*/);
  void setEndX(double /*endX*/);
  void setActiveSpectrum(WorkspaceIndex spectrum);
  void updatePlotSpectrum(WorkspaceIndex spectrum);
  void updateRangeSelectors();
  void appendLastDataToSelection(std::vector<std::string> displayNames);
  void updateDataSelection(std::vector<std::string> displayNames);
  void updateAvailableSpectra();
  void updatePlots();
  void updateFit();
  void updateGuess();
  void updateGuessAvailability();

signals:
  void selectedFitDataChanged(WorkspaceID /*_t1*/);
  void plotSpectrumChanged();
  void fitSingleSpectrum(WorkspaceID /*_t1*/, WorkspaceIndex /*_t2*/);
  void startXChanged(double /*_t1*/);
  void endXChanged(double /*_t1*/);
  void fwhmChanged(double /*_t1*/);
  void backgroundChanged(double /*_t1*/);

private:
  void disableAllDataSelection();
  void enableAllDataSelection();
  void plotInput(Mantid::API::MatrixWorkspace_sptr workspace);
  void plotInput(Mantid::API::MatrixWorkspace_sptr workspace, WorkspaceIndex spectrum);
  void plotFit(const Mantid::API::MatrixWorkspace_sptr &workspace);
  void plotFit(Mantid::API::MatrixWorkspace_sptr workspace, WorkspaceIndex spectrum);
  void plotDifference(Mantid::API::MatrixWorkspace_sptr workspace, WorkspaceIndex spectrum);
  void plotGuess(Mantid::API::MatrixWorkspace_sptr workspace);
  void plotLines();
  void updatePlotRange(const std::pair<double, double> &range);
  void clearGuess();
  void updateHWHMSelector();
  void setHWHM(double value);
  void updateBackgroundSelector();
  void updateFitRangeSelector();
  void emitSelectedFitDataChanged();
  void setActiveIndex(WorkspaceID workspaceID);

  void plotSpectrum(WorkspaceIndex spectrum) const;

  std::unique_ptr<IndirectFitPlotModel> m_model;
  IIndirectFitPlotView *m_view;

  std::unique_ptr<Widgets::MplCpp::ExternalPlotter> m_plotter;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
