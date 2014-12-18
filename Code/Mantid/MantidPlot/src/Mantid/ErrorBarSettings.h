#ifndef ERRORBARSETTINGS_H
#define ERRORBARSETTINGS_H

#include <QObject>
#include <QColor>

/** Holds the settings for how a set of error bars are to be drawn.
    The class is a QObject purely so that our python proxy objects will work with it.

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
*/
class ErrorBarSettings : public QObject
{
  Q_OBJECT
public:
  ErrorBarSettings(QObject * parent = 0);

  int capLength() const;
  void setCapLength(int t);

  virtual double width() const;
  virtual void setWidth(double w);

  virtual QColor color() const;
  virtual void setColor(const QColor& c);

  bool throughSymbol() const;
  void drawThroughSymbol(bool yes);

  bool plusSide() const;
  void drawPlusSide(bool yes);

  bool minusSide() const;
  void drawMinusSide(bool yes);

  QString toString() const;
  void fromString(const QString& settings);

private:
  int m_cap;      ///< Length of the bar cap decoration
  bool m_plus;    ///< Whether to draw these errors on the positive side
  bool m_minus;   ///< Whether to draw these errors on the negative side
  bool m_through; ///< Whether to draw through any symbol on the curve
  
  double m_width; ///< Width of the error bars (only used for Mantid error bars)
  QColor m_color;  ///< Color of the error bars (only used for Mantid error bars)

  bool m_defaultColor; ///< Whether the color has been explicitly set via setColor();

  friend class MantidCurve;
  friend class MantidMatrixCurve;
};

#endif
