#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTSYMMETRISE_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTSYMMETRISE_H_

#include "IndirectDataReductionTab.h"

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/System.h"
#include "ui_IndirectSymmetrise.h"

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
/** IndirectSymmetrise

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
class DLLExport IndirectSymmetrise : public IndirectDataReductionTab {
  Q_OBJECT

public:
  IndirectSymmetrise(IndirectDataReduction *idrUI, QWidget *parent = nullptr);
  ~IndirectSymmetrise() override;

  void setup() override;
  void run() override;
  bool validate() override;

private slots:
  void algorithmComplete(bool error);
  void plotRawInput(const QString &workspaceName);
  void updateMiniPlots();
  void replotNewSpectrum(QtProperty *prop, double value);
  void verifyERange(QtProperty *prop, double value);
  void updateRangeSelectors(QtProperty *prop, double value);
  void preview();
  void previewAlgDone(bool error);
  void xRangeMaxChanged(double value);
  void xRangeMinChanged(double value);
  void plotClicked();
  void saveClicked();

private:
  Ui::IndirectSymmetrise m_uiForm;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_INDIRECTSYMMETRISE_H_
