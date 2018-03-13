#include "MantidGeometry/Rendering/GeometryHandler.h"

namespace Mantid {
namespace Geometry {

/** Constructor
 *  @param[in] comp
 *  This geometry handler will be ObjComponent's geometry handler
 */
GeometryHandler::GeometryHandler(IObjComponent *comp) : csgObj() {
  ObjComp = comp;
  meshObj = nullptr;
  boolTriangulated = true;
  boolIsInitialized = false;
}

/** Constructor
 *  @param[in] obj
 *  This geometry handler will be Object's geometry handler
 */
GeometryHandler::GeometryHandler(boost::shared_ptr<CSGObject> obj)
    : csgObj(obj.get()) {
  ObjComp = nullptr;
  meshObj = nullptr;
  boolTriangulated = false;
  boolIsInitialized = false;
}

/** Constructor
 *  @param[in] obj
 *  This geometry handler will be Object's geometry handler
 */
GeometryHandler::GeometryHandler(CSGObject *obj) : csgObj(obj) {
  ObjComp = nullptr;
  meshObj = nullptr;
  boolTriangulated = false;
  boolIsInitialized = false;
}

/** Constructor
*  @param[in] obj
*  This geometry handler will be Object's geometry handler
*/
GeometryHandler::GeometryHandler(boost::shared_ptr<MeshObject> obj) : csgObj() {
  ObjComp = nullptr;
  meshObj = obj.get();
  boolTriangulated = false;
  boolIsInitialized = false;
}

/** Constructor
*  @param[in] obj
*  This geometry handler will be Object's geometry handler
*/
GeometryHandler::GeometryHandler(MeshObject *obj) : csgObj() {
  ObjComp = nullptr;
  meshObj = obj;
  boolTriangulated = false;
  boolIsInitialized = false;
}

/// Destructor
GeometryHandler::~GeometryHandler() {
  ObjComp = nullptr;
  meshObj = nullptr;
}
}
}
