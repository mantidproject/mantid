// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../DllConfig.h"
#include "DataReductionTab.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/System.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/IRunSubscriber.h"
#include "ui_ISISDiagnostics.h"

// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
#pragma warning disable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "MantidQtWidgets/Common/QtPropertyBrowser/QtCheckBoxFactory"
#if defined(__INTEL_COMPILER)
#pragma warning enable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
#endif

namespace MantidQt {
namespace CustomInterfaces {
class IDataReduction;

/** ISISDiagnostics
  Handles time integration diagnostics for ISIS instruments.

  @author Dan Nixon
  @date 23/07/2014
*/
class MANTIDQT_INDIRECT_DLL ISISDiagnostics : public DataReductionTab, public IRunSubscriber {
  Q_OBJECT

public:
  ISISDiagnostics(IDataReduction *idrUI, QWidget *parent = nullptr);
  ~ISISDiagnostics() override;

  void handleRun() override;
  void handleValidation(IUserInputValidator *validator) const override;

private slots:
  void algorithmComplete(bool error);
  void handleNewFile();
  void sliceTwoRanges(QtProperty * /*unused*/, bool /*state*/);
  void sliceCalib(bool state);
  void rangeSelectorDropped(double /*min*/, double /*max*/);
  void doublePropertyChanged(QtProperty * /*prop*/, double /*val*/);
  void sliceAlgDone(bool error);
  void pbRunFinding();  //< Called when the FileFinder starts finding the files.
  void pbRunFinished(); //< Called when the FileFinder has finished finding the
  // files.
  void saveClicked();

  void setSaveEnabled(bool enabled);

private:
  void updateInstrumentConfiguration() override;

  void setDefaultInstDetails(QMap<QString, QString> const &instrumentDetails);

  void setFileExtensionsByName(bool filter) override;

  void setPeakRangeLimits(double peakMin, double peakMax);
  void setBackgroundRangeLimits(double backgroundMin, double backgroundMax);
  void setRangeLimits(MantidWidgets::RangeSelector *rangeSelector, double minimum, double maximum,
                      QString const &minPropertyName, QString const &maxPropertyName);
  void setPeakRange(double minimum, double maximum);
  void setBackgroundRange(double minimum, double maximum);

  Ui::ISISDiagnostics m_uiForm;
};
} // namespace CustomInterfaces
} // namespace MantidQt
