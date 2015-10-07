#include "ErrorBarSettings.h"
#include <cassert>

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

/// Write the settings to a tab-separated string. Used when saving a project.
QString ErrorBarSettings::toString() const
{
  // Be sure to go through the (virtual) methods because some are overridden in a derived class
  QString s = QString::number(this->width())+"\t";
  s += QString::number(this->capLength())+"\t";
  s += this->color().name()+"\t";
  s += QString::number(this->throughSymbol())+"\t";
  s += QString::number(this->plusSide())+"\t";
  s += QString::number(this->minusSide());

  return s;
}

/// Set the attributes from a tab-separated string. Used when loading a project
void ErrorBarSettings::fromString(const QString& settings)
{
  const QStringList settingslist = settings.split("\t");
  // Try to spot if something changes upstream
  assert( settingslist.size() == 6 );
  if ( settingslist.size() != 6 ) return;

  this->setWidth(settingslist[0].toDouble());
  this->setCapLength(settingslist[1].toInt());
  this->setColor(settingslist[2]);
  this->drawThroughSymbol(settingslist[3].toInt());
  this->drawPlusSide(settingslist[4].toInt());
  this->drawMinusSide(settingslist[5].toInt());
}


