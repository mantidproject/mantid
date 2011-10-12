#include "CustomTools.h"
#include <iomanip>
#include <iosfwd>
#include <iostream>


/** Called each time the mouse moves over the canvas */
QwtText CustomPicker::trackerText(const QwtDoublePoint & pos) const
{
  const QPoint point = pos.toPoint();
  emit mouseMoved(pos.x(), pos.y());
  return QwtText();
}

