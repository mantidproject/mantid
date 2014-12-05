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

#ifndef SCALEDETAILS_H_
#define SCALEDETAILS_H_

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

class ScaleDetails: public QWidget
{
  Q_OBJECT
    //details for each axis in the Scale Tab
public:
  ScaleDetails(ApplicationWindow* app, Graph* graph, int mappedaxis, QWidget *parent = 0); // populate and fill in with existing data
  virtual ~ScaleDetails();
  void initWidgets();
  bool modified(){return m_modified;}
  void apply();
  bool valid();

public slots:
  void axisEnabled(bool enabled);

private slots:
  void radiosSwitched();
  void setModified();
  void recalcStepMin();

private:
  bool m_modified, m_initialised;
  int m_mappedaxis;
  ApplicationWindow* m_app;
  Graph* m_graph;

  DoubleSpinBox *m_dspnEnd, *m_dspnStart, *m_dspnStep, *m_dspnBreakStart, *m_dspnBreakEnd, *m_dspnStepBeforeBreak, *m_dspnStepAfterBreak;
  QCheckBox *m_chkInvert, *m_chkLog10AfterBreak, *m_chkBreakDecoration;
  QRadioButton *m_radStep, *m_radMajor;
  QSpinBox *m_spnMajorValue, *m_spnBreakPosition, *m_spnBreakWidth;
  QGroupBox *m_grpAxesBreaks;
  QComboBox *m_cmbMinorTicksBeforeBreak, *m_cmbMinorTicksAfterBreak, *m_cmbScaleType, *m_cmbMinorValue, *m_cmbUnit;
  QLabel *m_lblScaleTypeLabel, *m_lblMinorBox, *m_lblStart, *m_lblEnd;
  QDateTimeEdit *m_dteStartDateTime, *m_dteEndDateTime;
  QTimeEdit *m_timStartTime, *m_timEndTime;

  void checkstep();
  bool validate();
};

#endif /* ScaleDetails_H_ */
