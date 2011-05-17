#include "ProjectionSurface.h"
#include "GLColor.h"
#include "MantidGLWidget.h"
#include "OpenGLError.h"

#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/IInstrument.h"

#include <QRgb>
#include <QSet>
#include <QMenu>

#include <cfloat>
#include <limits>
#include <cmath>

/**
  * The constructor.
  * @param rootActor :: The instrument actor containning all info about the instrument
  * @param origin :: Defines the origin of the projection reference system (if applicable)
  * @param axis :: 
  */
ProjectionSurface::ProjectionSurface(const InstrumentActor* rootActor,const Mantid::Geometry::V3D& origin,const Mantid::Geometry::V3D& axis):
    m_instrActor(rootActor),
    m_pos(origin),
    m_zaxis(axis)
{
}

