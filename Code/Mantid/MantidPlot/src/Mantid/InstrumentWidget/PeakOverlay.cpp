#include "PeakOverlay.h"
#include "MantidAPI/IPeaksWorkspace.h"

#include <QPainter>
#include <QList>
#include <cmath>
#include <algorithm>

QList<PeakMarker2D::Style> PeakOverlay::g_defaultStyles;

/**
* Constructor.
*/
PeakHKL::PeakHKL(PeakMarker2D* m,const QRectF& trect,bool sr):
p(m->origin()),
  rect(trect),
  //rectTopLeft(m->getLabelRect().topLeft()),
  h(m->getH()),
  k(m->getK()),
  l(m->getL()),
  nh(true),
  nk(true),
  nl(true),
  showRows(sr)
{
  rows.append(m->getRow());
}

/**
* Check if this rect intersects with marker's and if it does combine the labels
* @param marker :: A marker to check for intersection
* @param trect :: Transformed marker's label rect
* @return True if labels were combined, false otherwise.
*/
bool PeakHKL::add(PeakMarker2D* marker,const QRectF& trect)
{
  if ( !rect.intersects(trect) )
  {
    return false;
  }
  if (nh && marker->getH() != h) 
  {
    nh = false;
  }
  if (nk && marker->getK() != k) 
  {
    nk = false;
  }
  if (nl && marker->getL() != l) 
  {
    nl = false;
  }
  rows.append(marker->getRow());
  return true;
}
/**
* Draw the label
* @param painter :: QPainter to draw with
* @param prec :: precision
*/
void PeakHKL::draw(QPainter& painter,int prec)
{
  QString label;
  if (nh) 
  {
    int max_prec = std::max(prec,int(log10(h)+1));
    label = QString::number(h,'g',max_prec) + " ";
  }
  else
    label = "h ";
  if (nk)
  {
    int max_prec = std::max(prec,int(log10(k)+1));
    label += QString::number(k,'g',max_prec) + " ";
  }
  else
    label += "k ";
  if (nl)
  {
    int max_prec = std::max(prec,int(log10(l)+1));
    label += QString::number(l,'g',max_prec);
  }
  else
    label += "l";
  if (showRows)
  {
    label += " [" + QString::number(rows[0]);
    for(int i=1; i < rows.size(); ++i)
    {
      label += "," + QString::number(rows[i]);
    }
    label += "]";
  }
  painter.drawText(rect.bottomLeft(),label);
}

void PeakHKL::print()const
{
  std::cerr << "     " << p.x() << ' ' << p.y() << '('<<h<<','<<k<<','<<l<<")("<<nh<<','<<nk<<','<<nl<<')' << std::endl;
}

/**
 * Constructor
 */
PeakOverlay::PeakOverlay(boost::shared_ptr<Mantid::API::IPeaksWorkspace> pws):
Shape2DCollection(),
m_peaksWorkspace(pws),
m_currentDefaultStyle(0),
m_precision(6),
m_showRows(true)
{
  if (g_defaultStyles.isEmpty())
  {
    g_defaultStyles << PeakMarker2D::Style(PeakMarker2D::Circle,Qt::red);
    g_defaultStyles << PeakMarker2D::Style(PeakMarker2D::Diamond,Qt::green);
    g_defaultStyles << PeakMarker2D::Style(PeakMarker2D::Square,Qt::magenta);
  }
}

/**
 * Not implemented yet.
 */
void PeakOverlay::removeShape(Shape2D*)
{
}

/**
 * Not implemented yet.
 */
void PeakOverlay::clear()
{
}

/**
 * Add new marker to the overlay.
 * @param m :: Pointer to the new marker
 */
void PeakOverlay::addMarker(PeakMarker2D* m)
{
  addShape(m,false);
  m_det2marker.insert(m->getDetectorID(),m);
}

/**
 * Draw peaks on screen.
 * @param painter :: The QPainter to draw with.
 */
void PeakOverlay::draw(QPainter& painter) const
{
  // Draw symbols
  Shape2DCollection::draw(painter);
  // Sort the labels to avoid overlapping
  QColor color;
  QRectF clipRect(painter.viewport());
  m_labels.clear();
  foreach(Shape2D* shape,m_shapes)
  {
    if (!clipRect.contains(m_transform.map(shape->origin()))) continue;
    PeakMarker2D* marker = dynamic_cast<PeakMarker2D*>(shape);
    if (!marker) continue;
    color = marker->getColor();
    QPointF p0 = marker->origin();
    QPointF p1 = m_transform.map(p0);
    QRectF rect = marker->getLabelRect();
    QPointF dp = rect.topLeft() - p0;
    p1 += dp;
    rect.moveTo(p1);

    //painter.setPen(color);
    //painter.drawRect(rect);

    bool overlap = false;
    // if current label overlaps with another
    // combine them substituting differing numbers with letter 'h','k', or 'l'
    for(int i = 0; i < m_labels.size(); ++i)
    {
      PeakHKL& hkl = m_labels[i];
      overlap = hkl.add(marker,rect);
    }
    
    if (!overlap)
    {
      PeakHKL hkl(marker,rect,m_showRows);
      m_labels.append(hkl);
    }
  }
  //std::cerr << m_labels.size() << " labels\n";
  painter.setPen(color);
  for(int i = 0; i < m_labels.size(); ++i)
  {
    PeakHKL& hkl = m_labels[i];
    hkl.draw(painter,m_precision);
    //hkl.print();
  }
}

/**
 * Return a list of markers put onto a detector
 * @param detID :: A detector ID for which markers are to be returned.
 * @return :: A list of zero ot more markers.
 */
QList<PeakMarker2D*> PeakOverlay::getMarkersWithID(int detID)const
{
  return m_det2marker.values(detID);
}

/**
 * Return the total number of peaks.
 */
int PeakOverlay::getNumberPeaks()const
{
  return m_peaksWorkspace->getNumberPeaks();
}

/**
 * Return the i-th peak.
 * @param i :: Peak index.
 * @return A reference to the peak.
 */
Mantid::API::IPeak& PeakOverlay::getPeak(int i)
{
  return m_peaksWorkspace->getPeak(i);
}

/**
 * Return a default style for creating markers and increment the style index. 
 * Styles are taken form g_defaultStyles
 */
PeakMarker2D::Style PeakOverlay::getNextDefaultStyle()const
{
  int i = m_currentDefaultStyle;
  ++m_currentDefaultStyle;
  m_currentDefaultStyle %= g_defaultStyles.size();
  return g_defaultStyles[i];
}
