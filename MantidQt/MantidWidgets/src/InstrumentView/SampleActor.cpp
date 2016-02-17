#include "MantidQtMantidWidgets/InstrumentView/SampleActor.h"
#include "MantidQtMantidWidgets/InstrumentView/InstrumentActor.h"
#include "MantidQtMantidWidgets/InstrumentView/OpenGLError.h"

#include "MantidAPI/Sample.h"
#include "MantidGeometry/IObjComponent.h"

using namespace Mantid;
using namespace Geometry;

namespace MantidQt {
namespace MantidWidgets {

SampleActor::SampleActor(const InstrumentActor &instrActor,
                         const Mantid::API::Sample &sample,
                         const ObjComponentActor *samplePosActor)
    : GLActor(), m_instrActor(instrActor), m_sample(sample),
      m_samplePosActor(samplePosActor),
      m_samplePos(samplePosActor->getObjComponent()), m_color(255, 255, 255) {}

/**
* Implementation of rendering Sample.
*/
void SampleActor::draw(bool picking) const {
  if (!picking && isVisible()) {
    OpenGLError::check("SampleActor::draw()");
    glPushAttrib(GL_ENABLE_BIT);
    GLboolean hasLight0;
    glGetBooleanv(GL_LIGHT0, &hasLight0);
    if (hasLight0) {
      glEnable(GL_LIGHTING);
    }
    glPushMatrix();
    m_color.paint();
    Mantid::Kernel::V3D pos = m_samplePos->getPos();
    glTranslated(pos.X(), pos.Y(), pos.Z());
    m_sample.getShape().draw();
    glPopMatrix();
    glPopAttrib();
    OpenGLError::check("SampleActor::draw()");
  }
}

void SampleActor::getBoundingBox(Mantid::Kernel::V3D &minBound,
                                 Mantid::Kernel::V3D &maxBound) const {
  Mantid::Geometry::BoundingBox boundBox = m_sample.getShape().getBoundingBox();
  minBound = boundBox.minPoint();
  maxBound = boundBox.maxPoint();
}
} // MantidWidgets
} // MantidQt
