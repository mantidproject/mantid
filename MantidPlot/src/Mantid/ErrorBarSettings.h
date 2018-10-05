// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ERRORBARSETTINGS_H
#define ERRORBARSETTINGS_H

#include <QColor>
#include <QObject>

/** Holds the settings for how a set of error bars are to be drawn.
    The class is a QObject purely so that our python proxy objects will work
   with it.
*/
class ErrorBarSettings : public QObject {
  Q_OBJECT
public:
  explicit ErrorBarSettings(QObject *parent = nullptr);

  int capLength() const;
  void setCapLength(int t);

  virtual double width() const;
  virtual void setWidth(double w);

  virtual QColor color() const;
  virtual void setColor(const QColor &c);

  bool throughSymbol() const;
  void drawThroughSymbol(bool yes);

  bool plusSide() const;
  void drawPlusSide(bool yes);

  bool minusSide() const;
  void drawMinusSide(bool yes);

  QString toString() const;
  void fromString(const QString &settings);

private:
  int m_cap;      ///< Length of the bar cap decoration
  bool m_plus;    ///< Whether to draw these errors on the positive side
  bool m_minus;   ///< Whether to draw these errors on the negative side
  bool m_through; ///< Whether to draw through any symbol on the curve

  double m_width; ///< Width of the error bars (only used for Mantid error bars)
  QColor m_color; ///< Color of the error bars (only used for Mantid error bars)

  bool m_defaultColor; ///< Whether the color has been explicitly set via
  /// setColor();

  friend class MantidCurve;
  friend class MantidMatrixCurve;
  friend class MantidMDCurve;
};

#endif
