#ifndef MANTIDPLOT_PEAKOVERLAY_H_
#define MANTIDPLOT_PEAKOVERLAY_H_

#include "Shape2DCollection.h"
#include "PeakMarker2D.h"

#include <QHash>

namespace Mantid{
  namespace API{
    class IPeak;
    class IPeaksWorkspace;
  }
}

class PeakMarker2D;

/**
 * Class for managing overlapping peak labels and drawing them on screen.
 * If labels of two or more peaks overlap they are combined into a single label.
 * A label shows three numbers h,k, and l. A combined label replaces non-equal
 * numbers of included markers with its letter.
 */
class PeakHKL
{
public:
  PeakHKL(PeakMarker2D* m,const QRectF& trect);
  bool add(PeakMarker2D* marker,const QRectF& trect);
  void draw(QPainter& painter,int prec = 6);
  void print()const;

private:
  QPointF p; ///< untransformed marker origin
  QRectF rect; ///< label's screen area in transformed coords
  double h,k,l; ///< h,k, and l
  bool nh,nk,nl; ///< true if h, k, or l is numeric

};

/**
 * Class for managing peak markers.
 */
class PeakOverlay: public Shape2DCollection
{
  Q_OBJECT
public:
  PeakOverlay(boost::shared_ptr<Mantid::API::IPeaksWorkspace> pws);
  ~PeakOverlay(){}
  /// Override the drawing method
  void draw(QPainter& painter) const;
  virtual void removeShape(Shape2D*);
  virtual void clear();

  void addMarker(PeakMarker2D* m);
  QList<PeakMarker2D*> getMarkersWithID(int detID)const;
  int getNumberPeaks()const;
  Mantid::API::IPeak& getPeak(int);
  PeakMarker2D::Style getNextDefaultStyle()const;
  /// Return PeaksWorkspace associated with this overlay.
  boost::shared_ptr<Mantid::API::IPeaksWorkspace> getPeaksWorkspace() {return m_peaksWorkspace;}
  /// set HKL precision
  void setPrecision(int prec) const {m_precision = prec;}

private:
  QMultiHash<int,PeakMarker2D*> m_det2marker; ///< detector ID to PeakMarker2D map
  mutable QList<PeakHKL> m_labels;
  boost::shared_ptr<Mantid::API::IPeaksWorkspace> m_peaksWorkspace; ///< peaks to be drawn ontop of the surface
  static QList<PeakMarker2D::Style> g_defaultStyles; ///< default marker styles
  mutable int m_currentDefaultStyle; ///< default style index
  mutable int m_precision;
};

#endif /*MANTIDPLOT_PEAKOVERLAY_H_*/
