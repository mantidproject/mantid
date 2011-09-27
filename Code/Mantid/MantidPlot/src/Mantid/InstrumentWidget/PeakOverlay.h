#ifndef MANTIDPLOT_PEAKOVERLAY_H_
#define MANTIDPLOT_PEAKOVERLAY_H_

#include "Shape2DCollection.h"
#include <QHash>

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
  void draw(QPainter& painter,const QTransform& transform);
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
public:
  PeakOverlay():Shape2DCollection(){}
  ~PeakOverlay(){}
  /// Override the drawing method
  void draw(QPainter& painter) const;

  void addMarker(PeakMarker2D* m);
  QList<PeakMarker2D*> getMarkersWithID(int detID)const;

private:
  QMultiHash<int,PeakMarker2D*> m_det2marker; ///< detector ID to PeakMarker2D map
  mutable QList<PeakHKL> m_labels;
};

#endif /*MANTIDPLOT_PEAKOVERLAY_H_*/
