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

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

//Foward decs
class QLabel;
class QLayout;
class QLineEdit;
class QComboBox;

// cppcheck-suppress class_X_Y
class EXPORT_OPT_MANTIDPARVIEW ThresholdRangeWidget: public QWidget
{

Q_OBJECT
public:
Q_PROPERTY(QString MinSignal READ getMinSignal WRITE setMinSignal NOTIFY minChanged)
Q_PROPERTY(QString MaxSignal READ getMaxSignal WRITE setMaxSignal NOTIFY maxChanged)
Q_PROPERTY(QString ChosenStrategy READ getChosenStrategy WRITE setChosenStrategy NOTIFY chosenStrategyChanged)

ThresholdRangeWidget(double min, double max);

~ThresholdRangeWidget();

QString getMaxSignal() const;
QString getMinSignal() const;
QString getChosenStrategy() const;

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

void setChosenStrategy(QString value)
{
  //Do nothing.
  UNUSED_ARG(value);
}


Q_SIGNALS:
        void minChanged();
        void maxChanged();
        void chosenStrategyChanged();

private:

  QLineEdit* m_maxEditBox;
  QLineEdit* m_minEditBox;
  QComboBox* m_thresholdStrategyComboBox;

  private slots:
  void maxThresholdListener(const QString &);

  void minThresholdListener(const QString &);

  void strategySelectedListener(const QString &);
};

#endif
