#ifndef GLU_GEOMETRYRENDERER_H
#define GLU_GEOMETRYRENDERER_H

#include "MantidKernel/System.h"
#include "MantidKernel/Logger.h"
namespace Mantid
{

  namespace Geometry
  {
    /*!
    \class GluGeometryRenderer
    \brief rendering geometry using opengl utility library glu.
    \author Srikanth Nagella
    \date July 2008
    \version 1.0

    This is an concrete class for rendering geometry using opengl.

    Copyright &copy; 2008 STFC Rutherford Appleton Laboratories

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
	class IObjComponent;
	class V3D;
	class DLLExport GluGeometryRenderer
    {
    private:

      static Kernel::Logger& PLog;           ///< The official logger
	  unsigned int           iDisplaylistId; ///< OpenGL display list id
	  bool                   boolDisplaylistCreated; ///< flag to store whether display list is created or not
	  int					 mErrorCode;
    public:
		GluGeometryRenderer();       ///< Constructor
		~GluGeometryRenderer();      ///< Destructor
		void Render(IObjComponent* ObjComp);
		void RenderSphere(V3D center,double radius);
		void RenderCube(V3D Point1,V3D Point2,V3D Point3,V3D Point4);
		void RenderCone(V3D center,V3D axis,double radius,double height);
		void RenderCylinder(V3D center,V3D axis,double radius,double height);
		void CreateSphere(V3D center,double radius);
		void CreateCube(V3D Point1,V3D Point2,V3D Point3,V3D Point4);
		void CreateCone(V3D center,V3D axis,double radius,double height);
		void CreateCylinder(V3D center,V3D axis,double radius,double height);
    };

  }   // NAMESPACE Geometry

}  // NAMESPACE Mantid

#endif
