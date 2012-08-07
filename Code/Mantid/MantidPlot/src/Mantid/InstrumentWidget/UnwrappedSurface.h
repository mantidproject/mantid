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

This class keeps information used to draw a detector on an unwrapped cylindrical surface.

*/
class UnwrappedDetector
{
public:
  UnwrappedDetector();
  UnwrappedDetector(const unsigned char* c,
                       boost::shared_ptr<const Mantid::Geometry::IDetector> det
                       );
  UnwrappedDetector(const UnwrappedDetector & other);
  const UnwrappedDetector & operator=(const UnwrappedDetector & other);
  unsigned char color[3]; ///< red, green, blue colour components (0 - 255)
  double u;      ///< horizontal "unwrapped" coordinate
  double v;      ///< vertical "unwrapped" coordinate
  double width;  ///< detector width in units of u
  double height; ///< detector height in units of v
  double uscale; ///< scaling factor in u direction
  double vscale; ///< scaling factor in v direction
  Mantid::Geometry::IDetector_const_sptr detector;
//  Mantid::Kernel::V3D minPoint,maxPoint;
};

/**
  * @class UnwrappedSurface
  * @brief Performs projection of an instrument onto a 2D surface and unwrapping it into a plane. Draws the resulting image
  *        on the screen.
  * @author Roman Tolchenov, Tessella plc
  * @date 18 Nov 2010
  */

class UnwrappedSurface: public ProjectionSurface
{
  //Q_OBJECT
public:
  UnwrappedSurface(const InstrumentActor* rootActor,const Mantid::Kernel::V3D& origin,const Mantid::Kernel::V3D& axis);
  ~UnwrappedSurface();
  void componentSelected(Mantid::Geometry::ComponentID = NULL);
  void getSelectedDetectors(QList<int>& dets);
  void getMaskedDetectors(QList<int>& dets)const;
  void setPeaksWorkspace(boost::shared_ptr<Mantid::API::IPeaksWorkspace> pws);
  virtual QString getInfoText()const;
  virtual QRectF getSurfaceBounds()const;
  void setFlippedView(bool on);
  bool isFlippedView() const {return m_flippedView;}

protected:
  virtual void drawSurface(MantidGLWidget* widget,bool picking = false)const;
  virtual void drawSimpleToImage(QImage* image,bool picking = false)const;
  virtual void changeColorMap();

  virtual void mousePressEventMove(QMouseEvent*);
  virtual void mouseMoveEventMove(QMouseEvent*);
  virtual void mouseReleaseEventMove(QMouseEvent*);
  virtual void wheelEventMove(QWheelEvent*);

  /// calculate and assign udet.u and udet.v
  virtual void project(double & u, double & v, double & uscale, double & vscale, const Mantid::Kernel::V3D & pos) const = 0;
  virtual void calcUV(UnwrappedDetector& udet, Mantid::Kernel::V3D & pos);

  /// calculate rotation R for a udet
  virtual void calcRot(const UnwrappedDetector& udet, Mantid::Kernel::Quat& R)const = 0;
  virtual double uPeriod()const{return 0.0;}

  void init();
  void calcSize(UnwrappedDetector& udet,const Mantid::Kernel::V3D& X,
                const Mantid::Kernel::V3D& Y);
  //void callback(boost::shared_ptr<const Mantid::Geometry::IDetector> det,const DetectorCallbackData& data);
  void setColor(int index,bool picking)const;
  void showPickedDetector();
  void calcAssemblies(const Mantid::Geometry::IComponent * comp,const QRectF& compRect);
  void cacheAllAssemblies();
  void findAndCorrectUGap();
  double applyUCorrection(double u)const;
  void createPeakShapes(const QRect& viewport)const;

  double m_u_min;                      ///< Minimum u
  double m_u_max;                      ///< Maximum u
  double m_v_min;                      ///< Minimum v
  double m_v_max;                      ///< Maximum v
  double m_height_max;  ///< Maximum detector height
  double m_width_max;   ///< Maximum detector width
  double m_u_correction;///< Correction to u calculated by project() after findAndCorrectUGap()

  /// Info needed to draw detectors onto unwrapped image
  std::vector<UnwrappedDetector> m_unwrappedDetectors;

  /// Bounding rectangles of detector assemblies
  QMap<Mantid::Geometry::ComponentID,QRectF> m_assemblies;

  bool m_flippedView; ///< if false the image is seen from the sample. if true the view is looking towards the sample.
  mutable bool m_startPeakShapes; ///< set to true to start creating m_peakShapes from m_peaksWorkspace, return to false after creation

};

#endif // UNWRAPPEDSURFACE_H
