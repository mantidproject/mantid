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

  Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
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
