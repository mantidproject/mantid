// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/**
This class holds the widgets that hold the details for each axis so the contents
are only filled once and switching axis only changes a pointer.

@author Keith Brown, Placement Student at ISIS Rutherford Appleton Laboratory
from the University of Derby
@date 15/09/2013
*/

#ifndef AXISDETAILS_H_
#define AXISDETAILS_H_

#include <QList>
#include <QWidget>
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

class AxisDetails : public QWidget {
  Q_OBJECT
  // details for each axis in the Axis tab
public:
  AxisDetails(
      ApplicationWindow *app, Graph *graph, int mappedaxis,
      QWidget *parent = nullptr); // populate and fill in with existing data
  ~AxisDetails() override;
  void initWidgets();
  bool modified() { return m_modified; }
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
  ApplicationWindow *m_app;
  Graph *m_graph;
  QCheckBox *m_chkShowAxis, *m_chkShowFormula;
  QGroupBox *m_grpTitle, *m_grpShowLabels, *m_grpAxisDisplay;
  QTextEdit *m_txtFormula, *m_txtTitle;
  QPushButton *m_btnLabelFont, *m_btnAxesFont;
  TextFormatButtons *m_formatButtons;
  QComboBox *m_cmbMajorTicksType, *m_cmbTableName, *m_cmbMinorTicksType,
      *m_cmbAxisType, *m_cmbFormat, *m_cmbColName;
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
