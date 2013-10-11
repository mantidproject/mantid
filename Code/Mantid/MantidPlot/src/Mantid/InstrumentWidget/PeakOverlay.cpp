#include "PeakOverlay.h"
#include "UnwrappedSurface.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/AlgorithmManager.h"

#include <QPainter>
#include <QList>
#include <cmath>
#include <algorithm>
#include <stdexcept>

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
      label += formatNumber( h, prec ) + " ";
  }
  else
    label = "h ";
  if (nk)
  {
      label += formatNumber( k, prec ) + " ";
  }
  else
    label += "k ";
  if (nl)
  {
    label += formatNumber( l, prec ) + " ";
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
  * Creates formated string for outputting h,k, or l
  *
  * @param h :: Value to output.
  * @param prec :: Precision as a number of decimal places.
  */
QString PeakHKL::formatNumber(double h, int prec)
{
    if (h == 0) return "0";
    int max_prec = std::max(prec,int(log10(h)+1));
    QString str = QString::number(h,'f',max_prec);
    if ( str.contains('.') )
    {
        while (str.endsWith('0')) str.chop(1);
        if (str.endsWith('.')) str.chop(1);
    }
    return str;
}

/**---------------------------------------------------------------------
 * Constructor
 */
PeakOverlay::PeakOverlay(UnwrappedSurface* surface, boost::shared_ptr<Mantid::API::IPeaksWorkspace> pws):
Shape2DCollection(),
m_peaksWorkspace(pws),
m_surface(surface),
m_precision(6),
m_showRows(true),
m_showLabels(true)
{
  if (g_defaultStyles.isEmpty())
  {
    g_defaultStyles << PeakMarker2D::Style(PeakMarker2D::Circle,Qt::red);
    g_defaultStyles << PeakMarker2D::Style(PeakMarker2D::Diamond,Qt::green);
    g_defaultStyles << PeakMarker2D::Style(PeakMarker2D::Square,Qt::magenta);
  }
  observeAfterReplace();
}

/**---------------------------------------------------------------------
 * Overridden virtual function to remove peaks from the workspace along with 
 * the shapes.
 * @param shapeList :: Shapes to remove.
 */
void PeakOverlay::removeShapes(const QList<Shape2D*>& shapeList)
{
  // vectors of rows to delete from the peaks workspace.
  std::vector<size_t> rows;
  foreach(Shape2D* shape, shapeList)
  {
    PeakMarker2D* marker = dynamic_cast<PeakMarker2D*>(shape);
    if ( !marker ) throw std::logic_error("Wrong shape type found.");
    rows.push_back( static_cast<size_t>( marker->getRow() ) );
  }

  // Run the DeleteTableRows algorithm to delete the peak.
  auto alg = Mantid::API::AlgorithmManager::Instance().create("DeleteTableRows",-1);
  alg->setPropertyValue("TableWorkspace", m_peaksWorkspace->name());
  alg->setProperty("Rows",rows);
  emit executeAlgorithm(alg);
}

/**---------------------------------------------------------------------
 * Not implemented yet.
 */
void PeakOverlay::clear()
{
  Shape2DCollection::clear();
  m_det2marker.clear();
}

/**---------------------------------------------------------------------
 * Add new marker to the overlay.
 * @param m :: Pointer to the new marker
 */
void PeakOverlay::addMarker(PeakMarker2D* m)
{
  addShape(m,false);
  m_det2marker.insert(m->getDetectorID(),m);
}

/**---------------------------------------------------------------------
 * Create the markers which graphically represent the peaks on the surface.
 * The coordinates of the Shape2DCollection must be set (calling setWindow())
 * prior calling this method.
 * @param style :: A style of drawing the markers.
 */
void PeakOverlay::createMarkers(const PeakMarker2D::Style& style)
{
  int nPeaks = getNumberPeaks();
  this->clear();
  for(int i = 0; i < nPeaks; ++i)
  {
    Mantid::API::IPeak& peak = getPeak(i);
    const Mantid::Kernel::V3D & pos = peak.getDetPos();
    // Project the peak (detector) position onto u,v coords
    double u,v, uscale, vscale;
    m_surface->project(pos, u,v, uscale, vscale );

    // Create a peak marker at this position
    PeakMarker2D* r = new PeakMarker2D(*this,u,v,style);
    r->setPeak(peak,i);
    addMarker(r);
  }
  deselectAll();
}

/**---------------------------------------------------------------------
 * Draw peaks on screen.
 * @param painter :: The QPainter to draw with.
 */
void PeakOverlay::draw(QPainter& painter) const
{
  // Draw symbols
  Shape2DCollection::draw(painter);

  if ( !m_showLabels ) return;

  // Sort the labels to avoid overlapping
  QColor color(Qt::red);
  if ( !m_shapes.isEmpty() )
  {
    color = m_shapes[0]->getColor();
  }
  QRectF clipRect(painter.viewport());
  m_labels.clear();
  foreach(Shape2D* shape,m_shapes)
  {
    if ( !shape->isVisible() ) continue;
    if (!clipRect.contains(m_transform.map(shape->origin()))) continue;
    PeakMarker2D* marker = dynamic_cast<PeakMarker2D*>(shape);
    if (!marker) continue;

    QPointF p0 = marker->origin();
    QPointF p1 = m_transform.map(p0);
    QRectF rect = marker->getLabelRect();
    QPointF dp = rect.topLeft() - p0;
    p1 += dp;
    rect.moveTo(p1);

    bool overlap = false;
    // if current label overlaps with another
    // combine them substituting differing numbers with letter 'h','k', or 'l'
    for(int i = 0; i < m_labels.size(); ++i)
    {
      PeakHKL& hkl = m_labels[i];
      overlap = hkl.add(marker,rect);
      if ( overlap ) break;
    }
    
    if (!overlap)
    {
      PeakHKL hkl(marker,rect,m_showRows);
      m_labels.append(hkl);
    }
  }
  //std::cerr << m_labels.size() << " labels " << color.red() << ' ' << color.green() << ' ' << color.blue() << "\n";
  painter.setPen(color);
  for(int i = 0; i < m_labels.size(); ++i)
  {
    PeakHKL& hkl = m_labels[i];
    hkl.draw(painter,m_precision);
    //hkl.print();
  }
}

/**---------------------------------------------------------------------
 * Return a list of markers put onto a detector
 * @param detID :: A detector ID for which markers are to be returned.
 * @return :: A list of zero ot more markers.
 */
QList<PeakMarker2D*> PeakOverlay::getMarkersWithID(int detID)const
{
  return m_det2marker.values(detID);
}

/**---------------------------------------------------------------------
 * Return the total number of peaks.
 */
int PeakOverlay::getNumberPeaks()const
{
  return m_peaksWorkspace->getNumberPeaks();
}

/** ---------------------------------------------------------------------
 * Return the i-th peak.
 * @param i :: Peak index.
 * @return A reference to the peak.
 */
Mantid::API::IPeak& PeakOverlay::getPeak(int i)
{
  return m_peaksWorkspace->getPeak(i);
}

/** ---------------------------------------------------------------------
 * Handler of the AfterReplace notifications. Updates the markers.
 * @param wsName :: The name of the modified workspace.
 * @param ws :: The shared pointer to the modified workspace.
 */
void PeakOverlay::afterReplaceHandle(const std::string& wsName,
  const Mantid::API::Workspace_sptr ws)
{
  Q_UNUSED(wsName);
  auto peaksWS = boost::dynamic_pointer_cast<Mantid::API::IPeaksWorkspace>( ws );
  if ( peaksWS && peaksWS == m_peaksWorkspace && m_surface )
  {
    auto style = isEmpty() ? getDefaultStyle(0) : m_det2marker.begin().value()->getStyle();
    clear();
    createMarkers( style );
    m_surface->requestRedraw(true);
  }
}

/** ---------------------------------------------------------------------
 * Return a default style for creating markers by index. 
 * Styles are taken form g_defaultStyles
 */
PeakMarker2D::Style PeakOverlay::getDefaultStyle(int index)
{
  index %= g_defaultStyles.size();
  return g_defaultStyles[index];
}

/** ---------------------------------------------------------------------
  * Set visibility of the peak markers according to the integration range
  * in the instrument actor.
  *
  * @param xmin :: The lower bound of the integration range.
  * @param xmax :: The upper bound of the integration range.
  * @param units :: Units of the x - array in the underlying workspace:
  *     "TOF", "dSpacing", or "Wavelength".
  */
void PeakOverlay::setPeakVisibility(double xmin, double xmax, QString units)
{
    enum XUnits {Unknown, TOF, dSpacing, Wavelength};
    XUnits xUnits = Unknown;
    if (units == "TOF")
        xUnits = TOF;
    else if (units == "dSpacing")
        xUnits = dSpacing;
    else if (units == "Wavelength")
        xUnits = Wavelength;
    foreach(Shape2D* shape,m_shapes)
    {
        PeakMarker2D* marker = dynamic_cast<PeakMarker2D*>(shape);
        if (!marker) continue;
        Mantid::API::IPeak& peak = getPeak(marker->getRow());
        double x = 0.0;
        switch (xUnits)
        {
        case TOF: x = peak.getTOF(); break;
        case dSpacing: x = peak.getDSpacing(); break;
        case Wavelength: x = peak.getWavelength(); break;
        // if unknown units always vidsible
        default: x = xmin;
        }
        bool on = x >= xmin && x <= xmax;
        marker->setVisible(on);
    }
}

