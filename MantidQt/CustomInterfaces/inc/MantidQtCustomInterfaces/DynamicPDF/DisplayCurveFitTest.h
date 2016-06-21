#ifndef MANTIDQTCUSTOMINTERFACES_DYNAMICPDF_DISPLAYCURVEFITTEST_H_
#define MANTIDQTCUSTOMINTERFACES_DYNAMICPDF_DISPLAYCURVEFITTEST_H_

// includes for interace functionailty
#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtAPI/UserSubWindow.h"
#include "ui_DisplayCurveFitTest.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace DynamicPDF {

/** An interface whose only purpose is to test widget DisplayCurveFit
  The interface is visible in MantidPlot only when compiled in Debug mode.

  @date 2016-02-22

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTIDQT_CUSTOMINTERFACES_DLL DisplayCurveFitTest
    : public MantidQt::API::UserSubWindow {
  Q_OBJECT

public:
  /// The name of the interface as registered into the factory
  static std::string name() { return "Test the DisplayCurveFit widget"; }
  // This interface's categories.
  static QString categoryInfo() { return "DynamicPDF"; }

  DisplayCurveFitTest(QWidget *parent = nullptr);
  ~DisplayCurveFitTest() override;

private slots:
  void loadSpectra(const QString &workspaceName);

private:
  void initLayout() override;
  /// The object containing the widgets defined in the form created in Qt
  /// Designer
  Ui::DisplayCurveFitTest m_uiForm;

}; // class DisplayCurveFitTest
} // namespace CustomInterfaces
} // namespace DynamicPDF
} // namespace MantidQt
#endif // MANTIDQTCUSTOMINTERFACES_DYNAMICPDF_DISPLAYCURVEFITTEST_H_
