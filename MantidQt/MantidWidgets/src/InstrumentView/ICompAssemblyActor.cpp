#include "MantidQtMantidWidgets/InstrumentView/ICompAssemblyActor.h"
#include <cfloat>

#include "MantidKernel/V3D.h"
#include "MantidGeometry/IComponent.h"

using namespace Mantid;
using namespace Geometry;

namespace MantidQt {
namespace MantidWidgets {
/**
* This is a constructor for CompAssembly Actor
* @param instrActor :: the instrument actor
* @param compID :: the component ID
*/
ICompAssemblyActor::ICompAssemblyActor(
    const InstrumentActor &instrActor,
    const Mantid::Geometry::ComponentID &compID)
    : ComponentActor(instrActor, compID), mNumberOfDetectors(0),
      minBoundBox(DBL_MAX, DBL_MAX, DBL_MAX),
      maxBoundBox(-DBL_MAX, -DBL_MAX, -DBL_MAX) {}

//------------------------------------------------------------------------------------------------
/**
* Return the bounding box
* @param minBound :: min point of the bounding box
* @param maxBound :: max point of the bounding box
*/
void ICompAssemblyActor::getBoundingBox(Mantid::Kernel::V3D &minBound,
                                        Mantid::Kernel::V3D &maxBound) const {
  minBound = minBoundBox;
  maxBound = maxBoundBox;
}
} // MantidWidgets
} // MantidQt
