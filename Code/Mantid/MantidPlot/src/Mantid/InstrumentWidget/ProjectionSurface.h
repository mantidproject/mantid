#ifndef PROJECTIONSURFACE_H
#define PROJECTIONSURFACE_H

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
class MantidGLWidget;

/**
  * @class ProjectionSurface
  * @brief Performs projection of an instrument onto a plane. Draws the resulting image on the screen.
  * @author Roman Tolchenov, Tessella plc
  * @date 13 May 2011
  */

class ProjectionSurface
{
  //Q_OBJECT
public:
  ProjectionSurface(const InstrumentActor* rootActor,const Mantid::Geometry::V3D& origin,const Mantid::Geometry::V3D& axis);
  virtual ~ProjectionSurface(){}
  virtual void startSelection(int x,int y) = 0;
  virtual void moveSelection(int x,int y) = 0;
  virtual void endSelection(int x,int y) = 0;
  //void zoom();
  virtual void zoom(const QRectF& area) = 0;
  virtual void unzoom() = 0;
  virtual void updateView() = 0;
  virtual void updateDetectors() = 0;
  virtual void draw(MantidGLWidget* widget) = 0;
  virtual void componentSelected(Mantid::Geometry::ComponentID) = 0;
  virtual void getPickedDetector(QSet<int>& dets) = 0;
  virtual int getDetectorID(int x, int y) = 0;
  virtual bool hasSelection()const = 0;

protected:
  const InstrumentActor* m_instrActor;
  const Mantid::Geometry::V3D m_pos;   ///< Origin (sample position)
  const Mantid::Geometry::V3D m_zaxis; ///< The z axis, symmetry axis of the cylinder
};

#endif // PROJECTIONSURFACE_H
