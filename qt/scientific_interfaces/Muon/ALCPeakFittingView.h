// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"

#include "DllConfig.h"
#include "IALCPeakFittingView.h"
#include "MantidQtWidgets/Plotting/PeakPicker.h"

#include "ui_ALCPeakFittingView.h"

#include <QWidget>

namespace MantidQt {
namespace CustomInterfaces {

/** ALCPeakFittingView : Qt implementation of the ALC Peak Fitting step
  interface.
*/
class MANTIDQT_MUONINTERFACE_DLL ALCPeakFittingView : public IALCPeakFittingView {
public:
  ALCPeakFittingView(QWidget *widget);
  ~ALCPeakFittingView();

  Mantid::API::IFunction_const_sptr function(std::string const &index) const override;
  std::optional<std::string> currentFunctionIndex() const override;
  Mantid::API::IPeakFunction_const_sptr peakPicker() const override;
  void emitFitRequested();

public slots:

  void initialize() override;
  void setDataCurve(Mantid::API::MatrixWorkspace_sptr workspace, std::size_t const &workspaceIndex = 0) override;
  void setFittedCurve(Mantid::API::MatrixWorkspace_sptr workspace, std::size_t const &workspaceIndex = 0) override;
  void setGuessCurve(Mantid::API::MatrixWorkspace_sptr workspace, std::size_t const &workspaceIndex = 0) override;
  void removePlot(QString const &plotName) override;
  void setFunction(const Mantid::API::IFunction_const_sptr &newFunction) override;
  void setParameter(std::string const &funcIndex, std::string const &paramName, double value) override;
  void setPeakPickerEnabled(bool enabled) override;
  void setPeakPicker(const Mantid::API::IPeakFunction_const_sptr &peak) override;
  void displayError(const QString &message) override;
  void help() override;
  void plotGuess() override;
  void changePlotGuessState(bool plotted) override;

private:
  /// The widget used
  QWidget *const m_widget;

  /// UI form
  Ui::ALCPeakFittingView m_ui;

  /// Peak picker tool - only one on the plot at any given moment
  MantidWidgets::PeakPicker *m_peakPicker;
};

} // namespace CustomInterfaces
} // namespace MantidQt
