#ifndef MANTIDQTCUSTOMINTERFACES_DYNAMICPDF_DISPLAYCONTROL_H_
#define MANTIDQTCUSTOMINTERFACES_DYNAMICPDF_DISPLAYCONTROL_H_

// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Mantid Headers from the same project
// Mantid headers from other projects
#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace.h"
// 3rd party library headers
#include <QObject>
// system headers

// Class forward declarations
namespace MantidQt {
namespace MantidWidgets {
class DisplayCurveFit;
class RangeSelector;
} // namespace MantidWidgets
namespace CustomInterfaces {
namespace DynamicPDF {
class InputDataControl;
}
} // namespace CustomInterfaces
} // namespace MantidQt

namespace MantidQt {
namespace CustomInterfaces {
namespace DynamicPDF {

/** Class to handle commands to the DisplayCurveFit widget

  @date 2016-03-17

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
class MANTIDQT_DYNAMICPDF_DLL DisplayControl : public QObject {
  Q_OBJECT

public:
  DisplayControl(InputDataControl *inputDataControl,
                 MantidQt::MantidWidgets::DisplayCurveFit *displayModelFit);
  ~DisplayControl();
  void init();
  std::pair<double, double> getFitMinMax();
  void setFitMin(const double &);
  void setFitMax(const double &);

public slots:
  void updateSliceForFitting();
  void rangeSelectorFitUpdated(const double &boundary);
  void updateModelEvaluationDisplay(const QString &workspaceName);

signals:
  void signalRangeSelectorFitUpdated();

private:
  /// object handling all input slices
  InputDataControl *m_inputDataControl;
  /// object handling the display of the data and fits
  MantidQt::MantidWidgets::DisplayCurveFit *m_displayModelFit;
  /// handy pointer to the fit-range selector
  MantidQt::MantidWidgets::RangeSelector *m_fitRangeSelector;
  /// workspace holding the cropped slice being displayed
  boost::shared_ptr<Mantid::API::MatrixWorkspace> m_dataShown;
  /// name of the workspace holding the cropped slice being displayed
  const std::string m_dataShownName;

}; // class DisplayControl
} // namespace DynamicPDF
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_DYNAMICPDF_INPUTDATACONTROL_H_
