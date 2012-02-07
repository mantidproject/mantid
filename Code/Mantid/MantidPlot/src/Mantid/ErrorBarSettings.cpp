#include "ErrorBarSettings.h"

/** Constructor.
 *  Sets defaults of black lines having width 1.0 and caps of length 6,
 *  that show on both sides of the symbol but don't draw through it.
 */
ErrorBarSettings::ErrorBarSettings(QObject * parent)
  : QObject(parent), m_cap(6), m_plus(true), m_minus(true), 
    m_through(false), m_width(1.0), m_color(Qt::black), m_defaultColor(true)
{}

/// Return the length of the cap decoration on the error bars
int ErrorBarSettings::capLength() const
{
  return m_cap;
}

/// Set the length of the cap decoration on the error bars
void ErrorBarSettings::setCapLength(int t)
{
  m_cap = t;
}

/// Returns the width of the error bar lines. Overridden in QwtErrorPlotCurve.
double ErrorBarSettings::width() const
{
  return m_width;
}

/// Sets the width of the error bar lines. Overridden in QwtErrorPlotCurve.
void ErrorBarSettings::setWidth(double w)
{
  m_width = w;
}

/// Returns the color of the error bars. Overridden in QwtErrorPlotCurve.
QColor ErrorBarSettings::color() const
{
  return m_color;
}

/// Sets the color of the error bars. Overridden in QwtErrorPlotCurve.
void ErrorBarSettings::setColor(const QColor& c)
{
  m_color = c;
  m_defaultColor = false;
}

/// Returns whether the error bar lines are drawn through any symbol
bool ErrorBarSettings::throughSymbol() const
{
  return m_through;
}

/// Sets whether to draw through any symbol
void ErrorBarSettings::drawThroughSymbol(bool yes)
{
  m_through=yes;
}

/// Returns whether these error bars will be drawn on the positive side
bool ErrorBarSettings::plusSide() const
{
  return m_plus;
}

/// Set whether these error bars will be drawn on the positive side
void ErrorBarSettings::drawPlusSide(bool yes)
{
  m_plus=yes;
}

/// Returns whether these error bars will be drawn on the negative side
bool ErrorBarSettings::minusSide() const
{
  return m_minus;
}

/// Set whether these error bars will be drawn on the negative side
void ErrorBarSettings::drawMinusSide(bool yes)
{
  m_minus=yes;
}
