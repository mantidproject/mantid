// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "InstrumentActor.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
#include "MantidQtWidgets/Common/WidgetDllOption.h"
#include "ProjectionSurface.h"
#include "UnwrappedDetector.h"
#include <memory>

#include <QImage>
#include <QList>
#include <QMap>
#include <QPainter>
#include <QRect>
#include <QSet>
#include <QStack>

namespace Mantid {
namespace Geometry {
class IDetector;
}
namespace API {
class IPeaksWorkspace;
}
} // namespace Mantid

class QGLWidget;
class GL3DWidget;

namespace MantidQt {
namespace MantidWidgets {
/**
 * @class UnwrappedSurface
 * @brief Performs projection of an instrument onto a 2D surface and unwrapping
 *it into a plane. Draws the resulting image
 *        on the screen.
 * @author Roman Tolchenov, Tessella plc
 * @date 18 Nov 2010
 *
 * Inherited classes must implement methods:
 *
 *   project(...)
 *   rotate(...)
 *   init()
 *
 * In init() the implementation must set values for:
 *
 *   m_u_min, m_u_max, m_v_min, m_v_max, m_height_max, m_width_max, m_viewRect,
 *   m_unwrappedDetectors, m_assemblies
 *
 */

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW UnwrappedSurface : public ProjectionSurface {
  Q_OBJECT
public:
  explicit UnwrappedSurface(const IInstrumentActor *rootActor, const QSize &widgetSize,
                            const bool maintainAspectRatio = true);
  UnwrappedSurface()
      : m_u_min(DBL_MAX), m_u_max(-DBL_MAX), m_v_min(DBL_MAX), m_v_max(-DBL_MAX), m_height_max(0), m_width_max(0),
        m_flippedView(false), m_startPeakShapes(false), m_maintainAspectRatio(true) {};

  /** @name Implemented public virtual methods */
  //@{
  void componentSelected(size_t componentIndex) override;
  void getSelectedDetectors(std::vector<size_t> &detIndices) override;
  void getMaskedDetectors(std::vector<size_t> &detIndices) const override;
  void getIntersectingDetectors(std::vector<size_t> &detIndices) const;
  void setPeaksWorkspace(const std::shared_ptr<Mantid::API::IPeaksWorkspace> &pws);
  QString getInfoText() const override;
  RectF getSurfaceBounds() const override;
  //@}

  /** @name New public virtual methods */
  //@{
  /**
   * Project a point in the 3D space onto the surface. The method returns the u-
   *and v- coordinates of the projection
   * as well as the scaling factors along the u and v axes. The scaling factors
   *help to draw an approximate projection
   * of a 3D object on the surface which is an orthographic projection of the
   *object onto the tagent plane to the
   * surface at point (uv) and scaled along u and v by the corresponding factor.
   *
   * @param detIndex :: The index of a detector in detectorInfo\componentInfo
   * @param u (output) :: u-coordinate of the projection.
   * @param v (output) :: v-coordinate of the projection.
   * @param uscale (output) :: The scaling factor along the u-coordinate.
   * @param vscale (output) :: The scaling factor along the v-coordinate.
   */
  virtual void project(const size_t detIndex, double &u, double &v, double &uscale, double &vscale) const = 0;
  //@}

  /*
   * Project a point in the 3D space onto the surface. The method returns the u-
   *and v- coordinates of the projection
   * as well as the scaling factors along the u and v axes. The scaling factors
   *help to draw an approximate projection
   * of a 3D object on the surface which is an orthographic projection of the
   *object onto the tagent plane to the
   * surface at point (uv) and scaled along u and v by the corresponding factor.
   *
   * @param position :: The position of a detector
   * @param u (output) :: u-coordinate of the projection.
   * @param v (output) :: v-coordinate of the projection.
   * @param uscale (output) :: The scaling factor along the u-coordinate.
   * @param vscale (output) :: The scaling factor along the v-coordinate.
   */
  virtual void project(const Mantid::Kernel::V3D &position, double &u, double &v, double &uscale,
                       double &vscale) const = 0;

  /** @name Public methods */
  //@{
  /// Toggle between the normal view and the "filpped" view (from behind)
  void setFlippedView(bool on);
  /// Is the surface showing the flipped view?
  bool isFlippedView() const { return m_flippedView; }
  /// Zoom into an area of the screen
  void zoom(const QRectF &area);
  //@}
  /// Load settings for the unwrapped surface from a project file
  virtual void loadFromProject(const std::string &lines) override;
  /// Save settings for the unwrapped surface to a project file
  virtual std::string saveToProject() const override;
  /// Get a handle to a peaks workspace from a name
  std::shared_ptr<Mantid::API::IPeaksWorkspace> retrievePeaksWorkspace(const std::string &name) const;

  /// Get the QRect of the top left corner of a detector, and its size, in terms of pixels.
  QRect detectorQRectInPixels(const std::size_t detectorIndex) const;

  void resize(int /*unused*/, int /*unused*/) override;

protected slots:

  /// Zoom into the area returned by selectionRectUV()
  void zoom();
  /// Unzoom view to the previous zoom area or to full view
  void unzoom();
  /// Reset the zoom to the full screen view
  void resetZoom();

protected:
  /** @name Implemented protected virtual methods */
  //@{
  void drawSurface(GLDisplay *widget, bool picking = false) const override;
  void drawSimpleToImage(QImage *image, bool picking = false) const override;
  void changeColorMap() override;
  //@}

  /** @name New protected virtual methods */
  //@{
  /**
   * Calculate a rotation needed to see a detector from the correct angle on the
   * surface.
   * The rotation should be such that the detector is seen from the tip of the
   * normal
   * to the surface at the detector's position.
   * @param udet :: A detector.
   * @param R :: The result rotaion.
   */
  virtual void rotate(const UnwrappedDetector &udet, Mantid::Kernel::Quat &R) const = 0;
  virtual void calcUV(UnwrappedDetector &udet);
  virtual void calcSize(UnwrappedDetector &udet);
  virtual QString getDimInfo() const;
  /// Called in non-picking drawSimpleToImage to draw something other than
  /// detectors
  /// Useful for debuging
  virtual void drawCustom(QPainter * /*unused*/) const {}
  //@}

  /** @name Protected methods */
  //@{
  void setColor(size_t index, bool picking) const;
  void createPeakShapes() const;
  //@}

  RectF selectionRectUV() const;
  RectF correctForAspectRatioAndZoom(const int widget_width, const int widget_height) const;

  double m_u_min;      ///< Minimum u
  double m_u_max;      ///< Maximum u
  double m_v_min;      ///< Minimum v
  double m_v_max;      ///< Maximum v
  double m_height_max; ///< Maximum detector height
  double m_width_max;  ///< Maximum detector width

  /// Info needed to draw detectors onto unwrapped image
  std::vector<UnwrappedDetector> m_unwrappedDetectors;

  bool m_flippedView; ///< if false the image is seen from the sample. if true
  /// the view is looking towards the sample.
  mutable bool m_startPeakShapes; ///< set to true to start creating
  /// m_peakShapes from m_peaksWorkspace, return
  /// to false after creation

  /// Zoom stack
  QStack<RectF> m_zoomStack;

  QSize m_widgetSize;

  bool m_maintainAspectRatio; ///< whether to maintain aspect ratio if widget has different aspect ratio to the data
};

} // namespace MantidWidgets
} // namespace MantidQt
