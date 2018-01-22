#include "MantidGeometry/Rendering/GeometryHandler.h"

namespace Mantid {
namespace Geometry {

/** Constructor
 *  @param[in] comp
 *  This geometry handler will be ObjComponent's geometry handler
 */
GeometryHandler::GeometryHandler(IObjComponent *comp) : Obj() {
  ObjComp = comp;
  boolTriangulated = true;
  boolIsInitialized = false;
}

/** Constructor
 *  @param[in] obj
 *  This geometry handler will be Object's geometry handler
 */
GeometryHandler::GeometryHandler(boost::shared_ptr<CSGObject> obj)
    : Obj(obj.get()) {
  ObjComp = nullptr;
  boolTriangulated = false;
  boolIsInitialized = false;
}

/** Constructor
 *  @param[in] obj
 *  This geometry handler will be Object's geometry handler
 */
GeometryHandler::GeometryHandler(CSGObject *obj) : Obj(obj) {
  ObjComp = nullptr;
  boolTriangulated = false;
  boolIsInitialized = false;
}

/// Destructor
GeometryHandler::~GeometryHandler() { ObjComp = nullptr; }
}
}
