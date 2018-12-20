#ifndef LABELTOOLLOGVALUESDIALOG_H_
#define LABELTOOLLOGVALUESDIALOG_H_

//----------------------------------
// Includes
//----------------------------------

#include "MantidSampleLogDialog.h"

//----------------------------------
// Forward declarations
//----------------------------------

class ApplicationWindow;
class QTreeWidgetItem;
class QTreeWidget;
class QPushButton;
class QRadioButton;
class QLabel;
class QLineEdit;
class QSpinBox;

/**
This class display a Sample Log Dialog for the Label Tool,
it is used to select a Sample Log and import it on the plot
as a label.

@author Dimitar Tasev, Mantid Development Team, STFC
@date 14/07/2016

Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class LabelToolLogValuesDialog : public SampleLogDialogBase {
  Q_OBJECT
public:
  /// Constructor
  LabelToolLogValuesDialog(const QString &wsname, QWidget *parentContainer,
                           Qt::WFlags flags = nullptr,
                           size_t experimentInfoIndex = 0);

  virtual ~LabelToolLogValuesDialog() override;

private slots:

  /// Override providing custom functionality to the import function
  virtual void importItem(QTreeWidgetItem *item) override;

private:
  /// Tracks which statistic of the log is selected
  QRadioButton *statRadioChoice[NUM_STATS];
};

#endif // LABELTOOLLOGVALUESDIALOG_H_