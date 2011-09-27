#include "PeakOverlay.h"
#include "PeakMarker2D.h"

#include <QPainter>
#include <QList>

/**
* Constructor.
*/
PeakHKL::PeakHKL(PeakMarker2D* m,const QRectF& trect):
p(m->origin()),
  rect(trect),
  //rectTopLeft(m->getLabelRect().topLeft()),
  h(m->getH()),
  k(m->getK()),
  l(m->getL()),
  nh(true),
  nk(true),
  nl(true)
{}

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
  return true;
}
/**
* Draw the label
* @param painter :: QPainter to draw with
* @param transform :: Current transform
*/
void PeakHKL::draw(QPainter& painter,const QTransform& transform)
{
  QString label;
  if (nh) label = QString::number(h) + " ";
  else
    label = "h ";
  if (nk) label += QString::number(k) + " ";
  else
    label += "k ";
  if (nl) label += QString::number(l);
  else
    label += "l";
  painter.drawText(rect.bottomLeft(),label);

}

void PeakHKL::print()const
{
  std::cerr << "     " << p.x() << ' ' << p.y() << '('<<h<<','<<k<<','<<l<<")("<<nh<<','<<nk<<','<<nl<<')' << std::endl;
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
      PeakHKL hkl(marker,rect);
      m_labels.append(hkl);
    }
  }
  //std::cerr << m_labels.size() << " labels\n";
  painter.setPen(color);
  for(int i = 0; i < m_labels.size(); ++i)
  {
    PeakHKL& hkl = m_labels[i];
    hkl.draw(painter,m_transform);
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
