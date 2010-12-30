#ifndef UNWRAPPEDSURFACE_H
#define UNWRAPPEDSURFACE_H

#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Quat.h"
#include "MantidGeometry/IComponent.h"
#include "InstrumentActor.h"
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
}

class GLColor;
class GL3DWidget;

/*!
\class UnwrappedDetector
\brief Class helper for drawing detectors on unwraped surfaces
\date 15 Nov 2010
\author Roman Tolchenov, Tessella plc

This class keeps information used to draw a detector on an unwrapped cylindrical surface.

*/
class UnwrappedDetector
{
public:
  UnwrappedDetector(const unsigned char* c,
                       boost::shared_ptr<const Mantid::Geometry::IDetector> det
                       );
  unsigned char color[3]; ///< red, green, blue colour components (0 - 255)
  double u;      ///< horizontal "unwrapped" coordinate
  double v;      ///< vertical "unwrapped" coordinate
  double width;  ///< detector width in units of u
  double height; ///< detector height in units of v
  double uscale; ///< scaling factor in u direction
  double vscale; ///< scaling factor in v direction
  boost::shared_ptr<const Mantid::Geometry::IDetector> detector;
//  Mantid::Geometry::V3D minPoint,maxPoint;
};

/**
  * @class UnwrappedSurface
  * @brief Performs projection of an instrument onto a 2D surface and unwrapping it into a plane. Draws the resulting image
  *        on the screen.
  * @author Roman Tolchenov, Tessella plc
  * @date 18 Nov 2010
  */

class UnwrappedSurface: public DetectorCallback
{
  //Q_OBJECT
public:
  UnwrappedSurface(const InstrumentActor* rootActor,const Mantid::Geometry::V3D& origin,const Mantid::Geometry::V3D& axis);
  ~UnwrappedSurface();
  void startSelection(int x,int y);
  void moveSelection(int x,int y);
  void endSelection(int x,int y);
  void zoom();
  void zoom(const QRectF& area);
  void unzoom();
  void updateView();
  void updateDetectors();

  void draw(GL3DWidget* widget);

  void componentSelected(Mantid::Geometry::ComponentID);
  void getPickedDetector(QSet<int>& dets);
  bool hasSelection()const;

protected:
  virtual void calcUV(UnwrappedDetector& udet) = 0;
  virtual void calcRot(UnwrappedDetector& udet, Mantid::Geometry::Quat& R) = 0;
  virtual void drawSurface(GL3DWidget* widget,bool picking = false);

  void init();
  void calcSize(UnwrappedDetector& udet,const Mantid::Geometry::V3D& X,
                const Mantid::Geometry::V3D& Y);
  void callback(boost::shared_ptr<const Mantid::Geometry::IDetector> det,const DetectorCallbackData& data);
  void clear();
  void setColor(int index,bool picking);
  void showPickedDetector();
  int getDetectorID(unsigned char r,unsigned char g,unsigned char b)const;
  QRect selectionRect()const;
  QRectF selectionRectUV()const;
  void calcAssemblies(boost::shared_ptr<const Mantid::Geometry::IComponent> comp,const QRectF& compRect);

  const InstrumentActor* m_instrActor;
  const Mantid::Geometry::V3D m_pos;   ///< Origin (sample position)
  const Mantid::Geometry::V3D m_zaxis; ///< The z axis, symmetry axis of the cylinder
  Mantid::Geometry::V3D m_xaxis;       ///< The x axis, defines the zero of the polar phi angle
  Mantid::Geometry::V3D m_yaxis;       ///< The y axis, rotation from x to y defines positive phi
  double m_u_min;                      ///< Minimum u
  double m_u_max;                      ///< Maximum u
  double m_v_min;                      ///< Minimum v
  double m_v_max;                      ///< Maximum v
  double m_height_max;  ///< Maximum detector height
  double m_width_max;   ///< Maximum detector width
  QImage* m_unwrappedImage;      ///< storage for unwrapped image
  QImage* m_pickImage;      ///< storage for picking image
  bool m_unwrappedViewChanged;   ///< set when the unwrapped image must be redrawn
  QList<UnwrappedDetector> m_unwrappedDetectors;  ///< info needed to draw detectors onto unwrapped image
  QMap<Mantid::Geometry::ComponentID,QRectF> m_assemblies;
  QRectF m_unwrappedView;
  QRect m_selectRect;
  QStack<QRectF> m_zoomStack;

  static double m_tolerance;     ///< tolerance for comparing 3D vectors

  void BasisRotation(const Mantid::Geometry::V3D& Xfrom,
                  const Mantid::Geometry::V3D& Yfrom,
                  const Mantid::Geometry::V3D& Zfrom,
                  const Mantid::Geometry::V3D& Xto,
                  const Mantid::Geometry::V3D& Yto,
                  const Mantid::Geometry::V3D& Zto,
                  Mantid::Geometry::Quat& R,
                  bool out = false
                  );
};

#endif // UNWRAPPEDSURFACE_H
