#ifndef GEOMETRYHANDLER_H
#define GEOMETRYHANDLER_H

#include "MantidKernel/System.h"
#include "MantidKernel/Logger.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Objects/Object.h"
//#include "boost/weak_ptr.hpp"

namespace Mantid
{

	namespace Geometry
	{
		/*!
		\class GeometryHandler
		\brief Place holder for geometry triangulation and rendering.
		\author Srikanth Nagella
		\date July 2008
		\version 1.0

		This is an abstract class for handling geometry primitives.

		Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

		This file is part of Mantid.

		Mantid is free software; you can redistribute it and/or modify
		it under the terms of the GNU General Public License as published by
		the Free Software Foundation; either version 3 of the License, or
		(at your option) any later version.

		Mantid is distributed in the hope that it will be useful,
		but WITHOUT ANY WARRANTY; without even the implied warranty of
		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
		GNU General Public License for more details.

		You should have received a copy of the GNU General Public License
		along with this program.  If not, see <http://www.gnu.org/licenses/>.

		File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
		*/
		class ObjComponent;
		class Object;
		class DLLExport GeometryHandler
		{
		private:
			static Kernel::Logger& PLog;           ///< The official logger

		protected:
			IObjComponent	*ObjComp;              ///< ObjComponent that uses this geometry handler
			Object          *Obj;                  ///< Object that uses this geometry handler
			bool			boolTriangulated;      ///< state of the geometry triangulation
			bool			boolIsInitialized;     ///< state of the geometry initialization for rendering
		public:
			GeometryHandler(IObjComponent *comp);   ///< Constructor
			GeometryHandler(boost::shared_ptr<Object> obj); ///<Constructor
			GeometryHandler(Object *obj); ///<Constructor
			virtual ~GeometryHandler();
			virtual GeometryHandler* createInstance(IObjComponent *)=0; ///< Create an instance of concrete geometry handler for ObjComponent
			virtual GeometryHandler* createInstance(boost::shared_ptr<Object> )=0; ///< Create an instance of concrete geometry handler for Object
			virtual void Triangulate()=0; ///< Triangulate the Object
			virtual void Render()=0;      ///< Render Object or ObjComponent
			virtual void Initialize()=0;  ///< Prepare/Initialize Object/ObjComponent to be rendered
			/// Returns true if the shape can be triangulated
			virtual bool    canTriangulate(){return false;}
			/// get the number of triangles
			virtual int     NumberOfTriangles(){return 0;}
			/// get the number of points or vertices
			virtual int     NumberOfPoints(){return 0;}
			/// Extract the vertices of the triangles
			virtual double* getTriangleVertices(){return NULL;}
			/// Extract the Faces of the triangles
			virtual int*    getTriangleFaces(){return NULL;}
			/// Sets the geometry cache using the triangulation information provided
			virtual void setGeometryCache(int noPts,int noFaces,double* pts,int* faces)
			{
        (void) noPts; (void) noFaces; (void) pts; (void) faces; //Avoid compiler warning
			};
			/// return the actual type and points of one of the "standard" objects, cuboid/cone/cyl/sphere
			virtual void GetObjectGeom(int& mytype, std::vector<Geometry::V3D>& vectors, double& myradius, double & myheight)
			{
        (void) mytype; (void) vectors; (void) myradius; (void) myheight; //Avoid compiler warning
			};
		};

	}   // NAMESPACE Geometry

}  // NAMESPACE Mantid

#endif
