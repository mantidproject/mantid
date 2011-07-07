#ifndef THRESHOLD_RANGE_WIDGET_H
#define THRESHOLD_RANGE_WIDGET_H

#include "WidgetDllOption.h"
#include <qwidget.h>
#include <qstring.h>
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include <qlineedit.h>
/** This is the GUI implementation of the threshold range widgets. These are used to set max and min threshold values.

    @author Owen Arnold Tessella/ISIS
    @date July 04/2011

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

//Foward decs
class QLabel;
class QLayout;
class QLineEdit;
class QCheckBox;

class EXPORT_OPT_MANTIDPARVIEW ThresholdRangeWidget: public QWidget
{

Q_OBJECT
public:
Q_PROPERTY(QString MinSignal READ getMinSignal WRITE setMinSignal NOTIFY minChanged)
Q_PROPERTY(QString MaxSignal READ getMaxSignal WRITE setMaxSignal NOTIFY maxChanged)
Q_PROPERTY(bool UserDefinedRange READ getUserDefinedRange WRITE setUserDefinedRange NOTIFY userDefinedChanged USER true)

ThresholdRangeWidget(double min, double max);

~ThresholdRangeWidget();

QString getMaxSignal() const;
QString getMinSignal() const;
bool getUserDefinedRange() const;

void setMaximum(double value);
void setMinimum(double value);

void setMinSignal(QString value)
{
  //Do nothing.
  UNUSED_ARG(value);
}

void setMaxSignal(QString value)
{
  //Do nothing.
  UNUSED_ARG(value);
}

void setUserDefinedRange(bool value)
{
  //Do nothing.
  UNUSED_ARG(value);
}

Q_SIGNALS:
        void minChanged();
        void maxChanged();
        void userDefinedChanged(bool checked);

    private:

  QLabel* m_minLabel;
  QLabel* m_maxLabel;
  QLineEdit* m_maxEditBox;
  QLineEdit* m_minEditBox;
  QCheckBox* m_ckUserDefined;

  private slots:
  void maxThresholdListener(const QString &);

  void minThresholdListener(const QString &);

  void methodChanged(bool);
};

#endif