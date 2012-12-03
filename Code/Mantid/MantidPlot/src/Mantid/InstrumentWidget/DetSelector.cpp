//--------------------------------
// Includes
//--------------------------------
#include "DetSelector.h"
#include "MantidKernel/System.h"

#include <QPainter>
#include <algorithm>

DetSelector::DetSelector():
m_xStart(0),m_yStart(0),m_inProgress(false),m_color(Qt::blue)
{
}

void DetSelector::draw(QPainter& painter)
{
  UNUSED_ARG(painter);
}

DetSelector* DetSelector::create(DetSelectionType type)
{
  switch(type)
  {
  case Single:
    return new DetSelector(); 
  default:
    return new BoxDetSelector();
  };
}

void DetSelector::start(int x, int y)
{
  m_xStart = x;
  m_yStart = y;
  m_inProgress = true;
}

void DetSelector::stop()
{
  m_inProgress = false;
}

void BoxDetSelector::draw(QPainter& painter)
{
  if (!m_inProgress) return;
  painter.setPen(m_color);
  painter.drawRect(m_xStart,m_yStart,m_xEnd,m_yEnd);
}

void BoxDetSelector::move(int x, int y)
{
  if (!m_inProgress) return;
  m_xEnd = x;
  m_yEnd = y;
  if (m_xEnd < m_xStart) std::swap(m_xEnd,m_xStart);
  if (m_yEnd < m_yStart) std::swap(m_yEnd,m_yStart);
}
