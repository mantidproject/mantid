/**
This class holds the widgets that hold the details for each axis so the contents are only filled once and switching axis only changes a pointer.

@author Keith Brown, Placement Student at ISIS Rutherford Appleton Laboratory from the University of Derby
@date 24/09/2013

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

#ifndef GRIDDETAILS_H_
#define GRIDDETAILS_H_

#include <QWidget>
#include <QList>
#include <DoubleSpinBox.h>
class ApplicationWindow;
class Graph;
class QCheckBox;
class ColorBox;
class QComboBox;
class DoubleSpinBox;
class Grid;
//The grid tab
class GridDetails: public QWidget
{
  Q_OBJECT
public:
  GridDetails(ApplicationWindow* app, Graph* graph, int alignment, QWidget *parent = 0); // populate and fill in with existing data
  virtual ~GridDetails();
  void initWidgets();
  bool modified(){return m_modified;}
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

  ApplicationWindow* m_app;
  Graph* m_graph;
  int m_alignment; //0 = horzontal, 1 = vertical, anything else sets this to 0;
};
#endif /* GRIDDETAILS_H_ */