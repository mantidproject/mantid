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
@date 24/09/2013
*/

#ifndef GRIDDETAILS_H_
#define GRIDDETAILS_H_

#include "MantidQtWidgets/Common/DoubleSpinBox.h"
#include <QList>
#include <QWidget>
class ApplicationWindow;
class Graph;
class QCheckBox;
class ColorBox;
class QComboBox;
class DoubleSpinBox;
class Grid;
// The grid tab
class GridDetails : public QWidget {
  Q_OBJECT
public:
  GridDetails(
      ApplicationWindow *app, Graph *graph, int alignment,
      QWidget *parent = nullptr); // populate and fill in with existing data
  ~GridDetails() override;
  void initWidgets();
  bool modified() { return m_modified; }
  void apply(Grid *grid, bool antialias, bool multirun = false);
public slots:
  void setModified();

private slots:
  void majorGridEnabled(bool on);
  void minorGridEnabled(bool on);

private:
  bool m_modified, m_initialised;
  QCheckBox *m_chkMajorGrid, *m_chkMinorGrid, *m_chkZeroLine;
  ColorBox *m_cboxColorMinor, *m_cboxColorMajor;
  QComboBox *m_cmbTypeMajor, *m_cmbTypeMinor, *m_cmbGridAxis;
  DoubleSpinBox *m_dspnWidthMajor, *m_dspnWidthMinor;

  ApplicationWindow *m_app;
  Graph *m_graph;
  int m_alignment; // 0 = horizontal, 1 = vertical, anything else sets this to
                   // 0;
};
#endif /* GRIDDETAILS_H_ */