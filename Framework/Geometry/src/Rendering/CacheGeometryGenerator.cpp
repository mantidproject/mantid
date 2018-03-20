#include <vector>
#include <cmath>
#include "MantidKernel/Matrix.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidGeometry/Rendering/CacheGeometryGenerator.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"

#ifdef ENABLE_OPENCASCADE
#include "MantidGeometry/Rendering/OCGeometryHandler.h"
#endif

namespace Mantid {

namespace Geometry {
/**
 * Constructor
 * @param obj :: input CSG object
 */
CacheGeometryGenerator::CacheGeometryGenerator(CSGObject *obj) : csgObj(obj) {
  mNoOfVertices = 0;
  mNoOfTriangles = 0;
  mFaces = nullptr;
  mPoints = nullptr;
  meshObj = nullptr;
}

/**
* Constructor
* @param obj :: input CSG object
*/
CacheGeometryGenerator::CacheGeometryGenerator(MeshObject *obj) : meshObj(obj) {
  mNoOfVertices = 0;
  mNoOfTriangles = 0;
  mFaces = nullptr;
  mPoints = nullptr;
  csgObj = nullptr;
}

/**
 * Generate geometry, if there is no cache then it uses OpenCascade to generate
 * surface triangles.
 */
void CacheGeometryGenerator::Generate() {
  if (mNoOfVertices <= 0) {
    if (csgObj != nullptr) {
// There are no triangles defined to use OpenCascade handler to get them
#ifdef ENABLE_OPENCASCADE
      OCGeometryHandler h(csgObj);
      mNoOfVertices = h.NumberOfPoints();
      mNoOfTriangles = h.NumberOfTriangles();
      mPoints = h.getTriangleVertices();
      mFaces = h.getTriangleFaces();
#endif
    } else if (meshObj != nullptr) {
      // Get triangles from MeshObject
      mNoOfVertices = meshObj->numberOfVertices();
      mNoOfTriangles = meshObj->numberOfTriangles();
      mPoints = meshObj->getVertices();
      mFaces = meshObj->getTriangles();
    }
  }
}

/**
 * Destroy the surface generated for the object
 */
CacheGeometryGenerator::~CacheGeometryGenerator() {
  if (mFaces != nullptr)
    delete[] mFaces;
  if (mPoints != nullptr)
    delete[] mPoints;
}

int CacheGeometryGenerator::getNumberOfTriangles() { return mNoOfTriangles; }

int CacheGeometryGenerator::getNumberOfPoints() { return mNoOfVertices; }

double *CacheGeometryGenerator::getTriangleVertices() { return mPoints; }

int *CacheGeometryGenerator::getTriangleFaces() { return mFaces; }

/**
   Sets the geometry cache using the triangulation information provided
   @param noPts :: the number of points
   @param noFaces :: the number of faces
   @param pts :: a double array of the points
   @param faces :: an int array of the faces
*/
void CacheGeometryGenerator::setGeometryCache(int noPts, int noFaces,
                                              double *pts, int *faces) {
  if (mPoints != nullptr)
    delete[] mPoints;
  if (mFaces != nullptr)
    delete[] mFaces;
  mNoOfVertices = noPts;
  mNoOfTriangles = noFaces;
  mPoints = pts;
  mFaces = faces;
}

} // NAMESPACE Geometry

} // NAMESPACE Mantid
