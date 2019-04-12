// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDPLOT_PEAKOVERLAY_H_
#define MANTIDPLOT_PEAKOVERLAY_H_

#include "PeakMarker2D.h"
#include "Shape2DCollection.h"

#include "MantidQtWidgets/Common/MantidAlgorithmMetatype.h"
#include "MantidQtWidgets/Common/WorkspaceObserver.h"

#include <QHash>
#include <QList>

///////////////////////////////////////////////////////////////////////////////
//     Forward declarations
///////////////////////////////////////////////////////////////////////////////
namespace Mantid {
namespace API {
class IPeak;
class IPeaksWorkspace;
} // namespace API
} // namespace Mantid

namespace MantidQt {
namespace MantidWidgets {

class UnwrappedSurface;

/**
 * Class for managing overlapping peak labels and drawing them on screen.
 * If labels of two or more peaks overlap they are combined into a single label.
 * A label shows three numbers h,k, and l. A combined label replaces non-equal
 * numbers of included markers with its letter.
 */
class PeakHKL {
public:
  PeakHKL(PeakMarker2D *m, const QRectF &trect, bool sr);
  bool add(PeakMarker2D *marker, const QRectF &trect);
  void draw(QPainter &painter, int prec = 6);
  void print() const;

private:
  static QString formatNumber(double h, int prec);
  QPointF p;       ///< untransformed marker origin
  QRectF rect;     ///< label's screen area in transformed coords
  double h, k, l;  ///< h,k, and l
  bool nh, nk, nl; ///< true if h, k, or l is numeric
  QList<int> rows; ///< row indices of the peaks in their PeaksWorkspace
  bool showRows;
};

/// Helper class for scaling peak markers to intensities.
class AbstractIntensityScale {
public:
  explicit AbstractIntensityScale(
      const boost::shared_ptr<Mantid::API::IPeaksWorkspace> &pws) {
    setPeaksWorkspace(pws);
  }

  virtual ~AbstractIntensityScale() {}

  void
  setPeaksWorkspace(const boost::shared_ptr<Mantid::API::IPeaksWorkspace> &pws);

  virtual PeakMarker2D::Style
  getScaledMarker(double intensity,
                  const PeakMarker2D::Style &baseStyle) const = 0;

protected:
  double m_maxIntensity = 0.0;
  double m_minIntensity = 0.0;
};

/// Default intensity scale leaves all markers unchanged.
class DefaultIntensityScale : public AbstractIntensityScale {
public:
  explicit DefaultIntensityScale(
      const boost::shared_ptr<Mantid::API::IPeaksWorkspace> &pws)
      : AbstractIntensityScale(pws) {}

protected:
  /// Returns the base style unmodified.
  PeakMarker2D::Style
  getScaledMarker(double intensity,
                  const PeakMarker2D::Style &baseStyle) const override {
    UNUSED_ARG(intensity);

    return baseStyle;
  }
};

/// Qualitative scaling of relative peak intensities to levels (weak, medium,
/// strong, very strong).
class QualitativeIntensityScale : public AbstractIntensityScale {
public:
  explicit QualitativeIntensityScale(
      const boost::shared_ptr<Mantid::API::IPeaksWorkspace> &pws)
      : AbstractIntensityScale(pws) {}

protected:
  PeakMarker2D::Style
  getScaledMarker(double intensity,
                  const PeakMarker2D::Style &baseStyle) const override;

private:
  // cppcheck-suppress unusedPrivateFunction
  int getIntensityLevel(double intensity) const;

  // Scaling to weak < 0.1 <= medium <= 0.6 <= strong <= 0.9 <= very strong
  std::vector<double> m_intensityLevels = {0.1, 0.6, 0.9};
};

/**
 * Class for managing peak markers on an unwrapped instrument surface.
 */
class PeakOverlay : public Shape2DCollection,
                    public MantidQt::API::WorkspaceObserver {
  Q_OBJECT
public:
  PeakOverlay(UnwrappedSurface *surface,
              boost::shared_ptr<Mantid::API::IPeaksWorkspace> pws);
  ~PeakOverlay() override {}
  /// Override the drawing method
  void draw(QPainter &painter) const override;
  void removeShapes(const QList<Shape2D *> & /*unused*/) override;
  void clear() override;

  /// Create the markers
  void createMarkers(const PeakMarker2D::Style &style);
  void addMarker(PeakMarker2D *m);
  QList<PeakMarker2D *> getMarkersWithID(int detID) const;
  int getNumberPeaks() const;
  Mantid::Geometry::IPeak &getPeak(int /*i*/);
  QList<PeakMarker2D *> getSelectedPeakMarkers();
  /// Return PeaksWorkspace associated with this overlay.
  boost::shared_ptr<Mantid::API::IPeaksWorkspace> getPeaksWorkspace() {
    return m_peaksWorkspace;
  }
  /// set HKL precision
  void setPrecision(int prec) const { m_precision = prec; }
  void setShowRowsFlag(bool yes) { m_showRows = yes; }
  void setShowLabelsFlag(bool yes) { m_showLabels = yes; }
  void setShowRelativeIntensityFlag(bool yes);
  static PeakMarker2D::Style getDefaultStyle(int index);
  void setPeakVisibility(double xmin, double xmax, QString units);

signals:
  void executeAlgorithm(Mantid::API::IAlgorithm_sptr /*_t1*/);

private:
  /// A WorkspaceObserver handle implemented.
  void afterReplaceHandle(const std::string &wsName,
                          const Mantid::API::Workspace_sptr ws) override;

  PeakMarker2D::Style getCurrentStyle() const;
  void recreateMarkers(const PeakMarker2D::Style &style);

  QMultiHash<int, PeakMarker2D *>
      m_det2marker; ///< detector ID to PeakMarker2D map
  mutable QList<PeakHKL> m_labels;
  boost::shared_ptr<Mantid::API::IPeaksWorkspace>
      m_peaksWorkspace; ///< peaks to be drawn ontop of the surface
  UnwrappedSurface
      *m_surface; ///< pointer to the surface this overlay is applied to
  mutable int m_precision;
  mutable bool m_showRows;   ///< flag to show peak row index
  mutable bool m_showLabels; ///< flag to show peak hkl labels

  std::unique_ptr<AbstractIntensityScale> m_peakIntensityScale;

  static QList<PeakMarker2D::Style> g_defaultStyles; ///< default marker styles
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /*MANTIDPLOT_PEAKOVERLAY_H_*/
