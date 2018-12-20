#ifndef MANTIDQT_CUSTOM_DIALOGS_GETNEGMUMUONICXRD_H_
#define MANTIDQT_CUSTOM_DIALOGS_GETNEGMUMUONICXRD_H_

#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "MantidQtWidgets/Common/PeriodicTableWidget.h"
#include <QDoubleSpinBox>
namespace MantidQt {

namespace CustomDialogs {
/**
This class gives a specialised dialog for the GetNegMuMuonicXRD algorithm
@author Matt King, ISIS Rutherford Appleton Laboratory
@date 11/08/2015
Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

class GetNegMuMuonicXRDDialog : public API::AlgorithmDialog {
  Q_OBJECT

public:
  /// Constructor
  GetNegMuMuonicXRDDialog(QWidget *parent = nullptr);

private:
  /// Periodic Table widget used for selection of elements property
  PeriodicTableWidget *m_periodicTable;
  /// QLineEdit used for input of y-position property
  QDoubleSpinBox *m_yPosition;
  /// QLineEdit used for input of GroupWorkspaceSpace
  QLineEdit *m_groupWorkspaceNameInput;
  // Check box for showing or hiding the Legend for PeriodicTableWidget
  QCheckBox *m_showLegendCheck;
  /** Enables a the buttons corresponding to the elements
  for which data exists in GetNegMuMuonicXRD.py
  */
  void enableElementsForGetNegMuMuonicXRD();

private slots:
  void parseInput() override;
  void showLegend();

protected:
  // create the initial layout
  void initLayout() override;
signals:
  /// signal emitted when validateDialogInput passes
  void validInput();
};
} // namespace CustomDialogs
} // namespace MantidQt
#endif // !MANTIDQT_CUSTOM_DIALOGS_GETNEGMUMUONICXRD_H_