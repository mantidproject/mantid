// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_ALCPEAKFITTINGVIEW_H_
#define MANTIDQT_CUSTOMINTERFACES_ALCPEAKFITTINGVIEW_H_

#include "MantidKernel/System.h"

#include "DllConfig.h"
#include "IALCPeakFittingView.h"
#include "MantidQtWidgets/Plotting/Qwt/PeakPicker.h"

#include "ui_ALCPeakFittingView.h"

#include <QWidget>
#include <qwt_plot_curve.h>

namespace MantidQt {
namespace MantidWidgets {
class ErrorCurve;
}
} // namespace MantidQt

namespace MantidQt {
namespace CustomInterfaces {

/** ALCPeakFittingView : Qt implementation of the ALC Peak Fitting step
  interface.
*/
class MANTIDQT_MUONINTERFACE_DLL ALCPeakFittingView
    : public IALCPeakFittingView {
public:
  ALCPeakFittingView(QWidget *widget);
  ~ALCPeakFittingView();

  // -- IALCPeakFitting interface
  // ----------------------------------------------------------------

  Mantid::API::IFunction_const_sptr function(QString index) const override;
  boost::optional<QString> currentFunctionIndex() const override;
  Mantid::API::IPeakFunction_const_sptr peakPicker() const override;
  void emitFitRequested();

public slots:

  void initialize() override;
  void setDataCurve(const QwtData &data,
                    const std::vector<double> &errors) override;
  void setFittedCurve(const QwtData &data) override;
  void
  setFunction(const Mantid::API::IFunction_const_sptr &newFunction) override;
  void setParameter(const QString &funcIndex, const QString &paramName,
                    double value) override;
  void setPeakPickerEnabled(bool enabled) override;
  void
  setPeakPicker(const Mantid::API::IPeakFunction_const_sptr &peak) override;
  void displayError(const QString &message) override;
  void help() override;
  void plotGuess() override;
  void changePlotGuessState(bool plotted) override;

  // -- End of IALCPeakFitting interface
  // ---------------------------------------------------------
private:
  /// The widget used
  QWidget *const m_widget;

  /// UI form
  Ui::ALCPeakFittingView m_ui;

  /// Plot curves
  QwtPlotCurve *m_dataCurve, *m_fittedCurve;

  /// Error curves
  MantidQt::MantidWidgets::ErrorCurve *m_dataErrorCurve;

  /// Peak picker tool - only one on the plot at any given moment
  MantidWidgets::PeakPicker *m_peakPicker;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_ALCPEAKFITTINGVIEW_H_ */
