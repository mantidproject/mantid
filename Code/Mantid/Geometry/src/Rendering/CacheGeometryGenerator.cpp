#include "MantidKernel/Logger.h"
#include <vector>
#include <cmath>
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Math/Matrix.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Rendering/CacheGeometryGenerator.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/OCGeometryHandler.h"

namespace Mantid
{

	namespace Geometry
	{
		Kernel::Logger& CacheGeometryGenerator::PLog(Kernel::Logger::get("CacheGeometryGenerator"));
		/**
		 * Constructor
		 * @param obj input object
		 */
		CacheGeometryGenerator::CacheGeometryGenerator(Object *obj):Obj(obj)
		{
			mNoOfVertices=0;
			mNoOfTriangles=0;
			mFaces=NULL;
			mPoints=NULL;
		}

		/**
		 * Generate geometry, if there is no cache then it uses OpenCascade to generate surface triangles.
		 */
		void CacheGeometryGenerator::Generate()
		{
			if(mNoOfVertices<=0) //There are no triangles defined to use OpenCascade handler
			{
				OCGeometryHandler h(Obj);
				mNoOfVertices=h.NumberOfPoints();
				mNoOfTriangles=h.NumberOfTriangles();
				mPoints=h.getTriangleVertices();
				mFaces=h.getTriangleFaces();
			}
		}

		/**
		 * Destroy the surface generated for the object
		 */
		CacheGeometryGenerator::~CacheGeometryGenerator()
		{
			if(mFaces!=NULL) delete[] mFaces;
			if(mPoints!=NULL) delete[] mPoints;
		}


    	int CacheGeometryGenerator::getNumberOfTriangles()
		{
			return mNoOfTriangles;
		}

		int CacheGeometryGenerator::getNumberOfPoints()
		{
			return mNoOfVertices;
		}

		double* CacheGeometryGenerator::getTriangleVertices()
		{
			return mPoints;
		}

		int* CacheGeometryGenerator::getTriangleFaces()
		{
			return mFaces;
		}

		/**
		Sets the geometry cache using the triangulation information provided
		@param noPts the number of points
		@param noFaces the number of faces
		@param pts a double array of the points
		@param faces an int array of the faces
		*/
		void CacheGeometryGenerator::setGeometryCache(int noPts,int noFaces,double* pts,int* faces)
		{
			if(mPoints!=NULL) delete[] mPoints;
			if(mFaces!=NULL) delete[] mFaces;
			mNoOfVertices=noPts;
			mNoOfTriangles=noFaces;
			mPoints=pts;
			mFaces=faces;
		}

	}  // NAMESPACE Geometry

}  // NAMESPACE Mantid
