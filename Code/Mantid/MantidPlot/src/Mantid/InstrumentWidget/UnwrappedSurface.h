#ifndef UNWRAPPEDSURFACE_H
#define UNWRAPPEDSURFACE_H

#include "MantidKernel/V3D.h"
#include "MantidKernel/Quat.h"
#include "MantidGeometry/IComponent.h"
#include "InstrumentActor.h"
#include "ProjectionSurface.h"
#include <boost/shared_ptr.hpp>

#include <QImage>
#include <QList>
#include <QStack>
#include <QSet>
#include <QMap>
#include <QPainter>

namespace Mantid{
  namespace Geometry{
    class IDetector;
  }
  namespace API{
    class IPeaksWorkspace;
  }
}

class GLColor;
class QGLWidget;
class GL3DWidget;

/**
\class UnwrappedDetector
\brief Class helper for drawing detectors on unwraped surfaces
\date 15 Nov 2010
\author Roman Tolchenov, Tessella plc

This class keeps information used to draw a detector on an unwrapped surface.

*/
class UnwrappedDetector
{
public:
  UnwrappedDetector();
  UnwrappedDetector(const unsigned char* c,
                       boost::shared_ptr<const Mantid::Geometry::IDetector> det
                       );
  UnwrappedDetector(const UnwrappedDetector & other);
  UnwrappedDetector & operator=(const UnwrappedDetector & other);
  unsigned char color[3]; ///< red, green, blue colour components (0 - 255)
  double u;      ///< horizontal "unwrapped" coordinate
  double v;      ///< vertical "unwrapped" coordinate
  double width;  ///< detector width in units of u
  double height; ///< detector height in units of v
  double uscale; ///< scaling factor in u direction
  double vscale; ///< scaling factor in v direction
  Mantid::Geometry::IDetector_const_sptr detector;
};

/**
  * @class UnwrappedSurface
  * @brief Performs projection of an instrument onto a 2D surface and unwrapping it into a plane. Draws the resulting image
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

class UnwrappedSurface: public ProjectionSurface
{
  Q_OBJECT
public:

  UnwrappedSurface(const InstrumentActor* rootActor);

  /** @name Implemented public virtual methods */
  //@{
  void componentSelected(Mantid::Geometry::ComponentID = NULL);
  void getSelectedDetectors(QList<int>& dets);
  void getMaskedDetectors(QList<int>& dets)const;
  void setPeaksWorkspace(boost::shared_ptr<Mantid::API::IPeaksWorkspace> pws);
  QString getInfoText()const;
  RectF getSurfaceBounds()const;
  //@}

  /** @name New public virtual methods */
  //@{
  /**
   * Project a point in the 3D space onto the surface. The method returns the u- and v- coordinates of the projection
   * as well as the scaling factors along the u and v axes. The scaling factors help to draw an approximate projection
   * of a 3D object on the surface which is an orthographic projection of the object onto the tagent plane to the
   * surface at point (uv) and scaled along u and v by the corresponding factor.
   *
   * @param pos :: A position of a 3D point.
   * @param u (output) :: u-coordinate of the projection.
   * @param v (output) :: v-coordinate of the projection.
   * @param uscale (output) :: The scaling factor along the u-coordinate.
   * @param vscale (output) :: The scaling factor along the v-coordinate.
   */
  virtual void project(const Mantid::Kernel::V3D & pos, double & u, double & v, double & uscale, double & vscale) const = 0;
  //@}

  /** @name Public methods */
  //@{
  /// Toggle between the normal view and the "filpped" view (from behind)
  void setFlippedView(bool on);
  /// Is the surface showing the flipped view?
  bool isFlippedView() const {return m_flippedView;}
  /// Zoom into an area of the screen
  void zoom(const QRectF& area);
  //@}

protected slots:

  /// Zoom into the area returned by selectionRectUV()
  void zoom();
  /// Unzoom view to the previous zoom area or to full view
  void unzoom();

protected:

  /** @name Implemented protected virtual methods */
  //@{
  void drawSurface(MantidGLWidget* widget,bool picking = false)const;
  void drawSimpleToImage(QImage* image,bool picking = false)const;
  void changeColorMap();
  //@}

  /** @name New protected virtual methods */
  //@{
  /**
   * Calculate a rotation needed to see a detector from the correct angle on the surface.
   * The rotation should be such that the detector is seen from the tip of the normal
   * to the surface at the detector's position.
   * @param udet :: A detector.
   * @param R :: The result rotaion.
   */
  virtual void rotate(const UnwrappedDetector& udet, Mantid::Kernel::Quat& R)const = 0;
  virtual void calcUV(UnwrappedDetector& udet, Mantid::Kernel::V3D & pos);
  virtual void calcSize(UnwrappedDetector& udet);
  virtual QString getDimInfo() const;
  /// Called in non-picking drawSimpleToImage to draw something other than detectors
  /// Useful for debuging
  virtual void drawCustom(QPainter*) const {}
  //@}

  /** @name Protected methods */
  //@{
  void setColor(int index,bool picking)const;
  void calcAssemblies(const Mantid::Geometry::IComponent * comp,const QRectF& compRect);
  void cacheAllAssemblies();
  void createPeakShapes(const QRect& viewport)const;
  //@}

  double m_u_min;       ///< Minimum u
  double m_u_max;       ///< Maximum u
  double m_v_min;       ///< Minimum v
  double m_v_max;       ///< Maximum v
  double m_height_max;  ///< Maximum detector height
  double m_width_max;   ///< Maximum detector width

  /// Info needed to draw detectors onto unwrapped image
  std::vector<UnwrappedDetector> m_unwrappedDetectors;

  /// Bounding rectangles of detector assemblies
  QMap<Mantid::Geometry::ComponentID,QRectF> m_assemblies;

  bool m_flippedView; ///< if false the image is seen from the sample. if true the view is looking towards the sample.
  mutable bool m_startPeakShapes; ///< set to true to start creating m_peakShapes from m_peaksWorkspace, return to false after creation

  /// Zoom stack
  QStack<RectF> m_zoomStack;
};

#endif // UNWRAPPEDSURFACE_H
