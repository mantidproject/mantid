// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"

#include "IndirectFitPlotModel.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidQtWidgets/Common/IndexTypes.h"
#include "MantidQtWidgets/Plotting/Mpl/ExternalPlotter.h"

#include "IIndirectFitPlotView.h"
#include "LazyAsyncRunner.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
using namespace MantidWidgets;

class MANTIDQT_INDIRECT_DLL IndirectFitPlotPresenter : public QObject {
  Q_OBJECT

public:
  IndirectFitPlotPresenter(IIndirectFitPlotView *view);

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

public slots:
  void setStartX(double /*startX*/);
  void setEndX(double /*endX*/);
  void updatePlotSpectrum(WorkspaceIndex spectrum);
  void updateRangeSelectors();
  void appendLastDataToSelection(std::vector<std::string> displayNames);
  void updateDataSelection(std::vector<std::string> displayNames);
  void updateAvailableSpectra();
  void updatePlots();
  void updateFit();
  void updateGuess();
  void updateGuessAvailability();
  void enablePlotGuessInSeparateWindow();
  void disablePlotGuessInSeparateWindow();
  void disableSpectrumPlotSelection();
  void handlePlotSpectrumChanged(WorkspaceIndex spectrum);
  void setActiveSpectrum(WorkspaceIndex spectrum);

signals:
  void selectedFitDataChanged(WorkspaceID /*_t1*/);
  void plotSpectrumChanged();
  void fitSingleSpectrum(WorkspaceID /*_t1*/, WorkspaceIndex /*_t2*/);
  void startXChanged(double /*_t1*/);
  void endXChanged(double /*_t1*/);
  void fwhmChanged(double /*_t1*/);
  void backgroundChanged(double /*_t1*/);

private slots:
  void setActiveIndex(WorkspaceID workspaceID);
  void setHWHMMaximum(double minimum);
  void setHWHMMinimum(double maximum);
  void plotGuess(bool doPlotGuess);
  void updateFitRangeSelector();
  void plotCurrentPreview();
  void emitFitSingleSpectrum();
  void emitFWHMChanged(double minimum, double maximum);
  void handleSelectedFitDataChanged(WorkspaceID workspaceID);

private:
  void disableAllDataSelection();
  void enableAllDataSelection();
  void plotInput(Mantid::API::MatrixWorkspace_sptr workspace);
  void plotInput(Mantid::API::MatrixWorkspace_sptr workspace, WorkspaceIndex spectrum);
  void plotFit(const Mantid::API::MatrixWorkspace_sptr &workspace);
  void plotFit(Mantid::API::MatrixWorkspace_sptr workspace, WorkspaceIndex spectrum);
  void plotDifference(Mantid::API::MatrixWorkspace_sptr workspace, WorkspaceIndex spectrum);
  void clearInput();
  void clearFit();
  void clearDifference();
  void plotGuess(Mantid::API::MatrixWorkspace_sptr workspace);
  void plotGuessInSeparateWindow(const Mantid::API::MatrixWorkspace_sptr &workspace);
  void plotLines();
  void updatePlotRange(const std::pair<double, double> &range);
  void clearGuess();
  void updateHWHMSelector();
  void setHWHM(double value);
  void updateBackgroundSelector();
  void emitSelectedFitDataChanged();

  void plotSpectrum(WorkspaceIndex spectrum) const;

  std::unique_ptr<IndirectFitPlotModel> m_model;
  IIndirectFitPlotView *m_view;

  bool m_plotGuessInSeparateWindow;
  QtLazyAsyncRunner<std::function<void()>> m_plotExternalGuessRunner;
  std::unique_ptr<Widgets::MplCpp::ExternalPlotter> m_plotter;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
