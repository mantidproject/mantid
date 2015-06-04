#ifndef MANTIDPLOT_PEAKOVERLAY_H_
#define MANTIDPLOT_PEAKOVERLAY_H_

#include "Shape2DCollection.h"
#include "PeakMarker2D.h"
#include "../MantidAlgorithmMetatype.h"

#include "MantidQtAPI/WorkspaceObserver.h"

#include <QHash>
#include <QList>

///////////////////////////////////////////////////////////////////////////////
//     Forward declarations
///////////////////////////////////////////////////////////////////////////////
namespace Mantid{
  namespace API{
    class IPeak;
    class IPeaksWorkspace;
  }
}

class UnwrappedSurface;

/**
 * Class for managing overlapping peak labels and drawing them on screen.
 * If labels of two or more peaks overlap they are combined into a single label.
 * A label shows three numbers h,k, and l. A combined label replaces non-equal
 * numbers of included markers with its letter.
 */
class PeakHKL
{
public:
  PeakHKL(PeakMarker2D* m,const QRectF& trect,bool sr);
  bool add(PeakMarker2D* marker,const QRectF& trect);
  void draw(QPainter& painter,int prec = 6);
  void print()const;

private:
  static QString formatNumber(double h, int prec);
  QPointF p; ///< untransformed marker origin
  QRectF rect; ///< label's screen area in transformed coords
  double h,k,l; ///< h,k, and l
  bool nh,nk,nl; ///< true if h, k, or l is numeric
  QList<int> rows; ///< row indices of the peaks in their PeaksWorkspace
  bool showRows;
};

/**
 * Class for managing peak markers on an unwrapped instrument surface.
 */
class PeakOverlay: public Shape2DCollection, public MantidQt::API::WorkspaceObserver
{
  Q_OBJECT
public:
  PeakOverlay(UnwrappedSurface* surface, boost::shared_ptr<Mantid::API::IPeaksWorkspace> pws);
  ~PeakOverlay(){}
  /// Override the drawing method
  void draw(QPainter& painter) const;
  virtual void removeShapes(const QList<Shape2D*>&);
  virtual void clear();

  /// Create the markers
  void createMarkers(const PeakMarker2D::Style& style);
  void addMarker(PeakMarker2D* m);
  QList<PeakMarker2D*> getMarkersWithID(int detID)const;
  int getNumberPeaks()const;
  Mantid::Geometry::IPeak& getPeak(int);
  /// Return PeaksWorkspace associated with this overlay.
  boost::shared_ptr<Mantid::API::IPeaksWorkspace> getPeaksWorkspace() {return m_peaksWorkspace;}
  /// set HKL precision
  void setPrecision(int prec) const {m_precision = prec;}
  void setShowRowsFlag(bool yes) {m_showRows = yes;}
  void setShowLabelsFlag(bool yes) {m_showLabels = yes;}
  static PeakMarker2D::Style getDefaultStyle(int index);
  void setPeakVisibility(double xmin, double xmax, QString units);

signals:
  void executeAlgorithm(Mantid::API::IAlgorithm_sptr);

private:
  /// A WorkspaceObserver handle implemented.
  virtual void afterReplaceHandle(const std::string& wsName,
    const Mantid::API::Workspace_sptr ws);

  QMultiHash<int,PeakMarker2D*> m_det2marker; ///< detector ID to PeakMarker2D map
  mutable QList<PeakHKL> m_labels;
  boost::shared_ptr<Mantid::API::IPeaksWorkspace> m_peaksWorkspace; ///< peaks to be drawn ontop of the surface
  UnwrappedSurface* m_surface; ///< pointer to the surface this overlay is applied to
  mutable int m_precision;
  mutable bool m_showRows;   ///< flag to show peak row index
  mutable bool m_showLabels; ///< flag to show peak hkl labels

  static QList<PeakMarker2D::Style> g_defaultStyles; ///< default marker styles
};

#endif /*MANTIDPLOT_PEAKOVERLAY_H_*/
