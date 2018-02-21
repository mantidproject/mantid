#include <vector>
#include <cmath>
#include "MantidKernel/Matrix.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidGeometry/Rendering/MeshGeometryGenerator.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"

#ifdef ENABLE_OPENCASCADE
#include "MantidGeometry/Rendering/OCGeometryHandler.h"
#endif

namespace Mantid {

namespace Geometry {
/**
 * Constructor
 * @param obj :: input object
 */
MeshGeometryGenerator::MeshGeometryGenerator(MeshObject *obj) : Obj(obj) {
  mNoOfVertices = 0;
  mNoOfTriangles = 0;
  mFaces = nullptr;
  mPoints = nullptr;
}

/**
 * Generate geometry, get triangles from object if not in cache.
 */
void MeshGeometryGenerator::Generate() {
  if (mNoOfVertices <= 0) { // Get triangles from object
  }
}

/**
 * Destroy the surface generated for the object
 */
MeshGeometryGenerator::~MeshGeometryGenerator() {
  if (mFaces != nullptr)
    delete[] mFaces;
  if (mPoints != nullptr)
    delete[] mPoints;
}

int MeshGeometryGenerator::getNumberOfTriangles() { return mNoOfTriangles; }

int MeshGeometryGenerator::getNumberOfPoints() { return mNoOfVertices; }

double *MeshGeometryGenerator::getTriangleVertices() { return mPoints; }

int *MeshGeometryGenerator::getTriangleFaces() { return mFaces; }

/**
   Sets the geometry cache using the triangulation information provided
   @param noPts :: the number of points
   @param noFaces :: the number of faces
   @param pts :: a double array of the points
   @param faces :: an int array of the faces
*/
void MeshGeometryGenerator::setGeometryCache(int noPts, int noFaces,
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
