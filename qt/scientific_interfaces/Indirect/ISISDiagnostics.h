// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_ISISDIAGNOSTICS_H_
#define MANTIDQTCUSTOMINTERFACES_ISISDIAGNOSTICS_H_

#include "IndirectDataReductionTab.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/System.h"
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
#include <MantidQtWidgets/Common/QtPropertyBrowser/QtCheckBoxFactory>
#if defined(__INTEL_COMPILER)
#pragma warning enable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
#endif

namespace MantidQt {
namespace CustomInterfaces {
/** ISISDiagnostics
  Handles time integration diagnostics for ISIS instruments.

  @author Dan Nixon
  @date 23/07/2014
*/
class DLLExport ISISDiagnostics : public IndirectDataReductionTab {
  Q_OBJECT

public:
  ISISDiagnostics(IndirectDataReduction *idrUI, QWidget *parent = nullptr);
  ~ISISDiagnostics() override;

  void setup() override;
  void run() override;
  bool validate() override;

private slots:
  void algorithmComplete(bool error);
  void handleNewFile();
  void sliceTwoRanges(QtProperty *, bool);
  void sliceCalib(bool state);
  void rangeSelectorDropped(double, double);
  void doublePropertyChanged(QtProperty *, double);
  void setDefaultInstDetails();
  void sliceAlgDone(bool error);
  void
  pbRunEditing(); //< Called when a user starts to type / edit the runs to load.
  void pbRunFinding();  //< Called when the FileFinder starts finding the files.
  void pbRunFinished(); //< Called when the FileFinder has finished finding the
  // files.
  void saveClicked();
  void plotClicked();

private:
  Ui::ISISDiagnostics m_uiForm;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ISISDIAGNOSTICS_H_
