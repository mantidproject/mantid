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

#ifndef SCALEDETAILS_H_
#define SCALEDETAILS_H_

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

class ScaleDetails : public QWidget {
  Q_OBJECT
  // details for each axis in the Scale Tab
public:
  ScaleDetails(
      ApplicationWindow *app, Graph *graph, int mappedaxis,
      QWidget *parent = nullptr); // populate and fill in with existing data
  ~ScaleDetails() override;
  void initWidgets();
  bool modified() { return m_modified; }
  void apply();
  bool valid();

public slots:
  void axisEnabled(bool enabled);

private slots:
  void radiosSwitched();
  void setModified();
  void recalcStepMin();
  void checkscaletype();

private:
  bool m_modified, m_initialised;
  int m_mappedaxis;
  ApplicationWindow *m_app;
  Graph *m_graph;

  DoubleSpinBox *m_dspnEnd, *m_dspnStart, *m_dspnStep, *m_dspnBreakStart,
      *m_dspnBreakEnd, *m_dspnStepBeforeBreak, *m_dspnStepAfterBreak, *m_dspnN;
  QCheckBox *m_chkInvert, *m_chkLog10AfterBreak, *m_chkBreakDecoration;
  QRadioButton *m_radStep, *m_radMajor;
  QSpinBox *m_spnMajorValue, *m_spnBreakPosition, *m_spnBreakWidth;
  QGroupBox *m_grpAxesBreaks;
  QComboBox *m_cmbMinorTicksBeforeBreak, *m_cmbMinorTicksAfterBreak,
      *m_cmbScaleType, *m_cmbMinorValue, *m_cmbUnit;
  QLabel *m_lblScaleTypeLabel, *m_lblMinorBox, *m_lblStart, *m_lblEnd, *m_lblN,
      *m_lblWarn;
  QDateTimeEdit *m_dteStartDateTime, *m_dteEndDateTime;
  QTimeEdit *m_timStartTime, *m_timEndTime;

  void checkstep();
  bool validate();
};

#endif /* ScaleDetails_H_ */
