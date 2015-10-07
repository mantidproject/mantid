/**
This class holds the widgets that hold the details for each axis so the contents are only filled once and switching axis only changes a pointer.

@author Keith Brown, Placement Student at ISIS Rutherford Appleton Laboratory from the University of Derby
@date 15/09/2013

Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

#ifndef AXISDETAILS_H_
#define AXISDETAILS_H_

#include <QWidget>
#include <QList>
class ApplicationWindow;
class QTimeEdit;
class QDateTimeEdit;
class QCheckBox;
class QGroupBox;
class QComboBox;
class QLabel;
class QRadioButton;
class QSpinBox;
class QWidget;
class QTextEdit;
class QPushButton;
class QStringList;
class DoubleSpinBox;
class Graph;
class TextFormatButtons;
class ColorButton;

class AxisDetails: public QWidget
{
  Q_OBJECT
    //details for each axis in the Axis tab
public:
  AxisDetails(ApplicationWindow* app, Graph* graph, int mappedaxis, QWidget *parent = 0); // populate and fill in with existing data
  virtual ~AxisDetails();
  void initWidgets();
  bool modified(){return m_modified;}
  void apply();
  bool valid();

signals:
  void axisShowChanged(bool enabled);
    
private slots:
  void enableFormulaBox();
  void showAxis();
  void setLabelFont();
  void setScaleFont();
  void setAxisFormatOptions(int format);
  void setModified();

private:
  bool m_modified, m_initialised;
  int m_mappedaxis;
  ApplicationWindow* m_app;
  Graph* m_graph;
  QCheckBox *m_chkShowAxis, *m_chkShowFormula;
  QGroupBox *m_grpTitle, *m_grpShowLabels, *m_grpAxisDisplay;
  QTextEdit *m_txtFormula, *m_txtTitle;
  QPushButton *m_btnLabelFont, *m_btnAxesFont;
  TextFormatButtons *m_formatButtons;
  QComboBox *m_cmbMajorTicksType, *m_cmbTableName, *m_cmbMinorTicksType, *m_cmbAxisType, *m_cmbFormat, *m_cmbColName;
  ColorButton *m_cbtnAxisColor, *m_cbtnAxisNumColor;
  QSpinBox *m_spnPrecision, *m_spnAngle, *m_spnBaseline;
  QLabel *m_lblColumn, *m_lblFormat, *m_lblPrecision, *m_lblTable;
  QFont m_labelFont, m_scaleFont;
  QStringList m_tablesList;

  void setEnabled();
  void updateAxisType(int axis);
  void updateTitleBox(int axis);
  void updateShowBox(int axis);
  void updateAxisColor(int);
  void setTicksType(int);
  void setLabelsNumericFormat(int);
  void updateLabelsFormat(int);
  void setBaselineDist(int);
};
#endif /* AxisDetails_H_ */
