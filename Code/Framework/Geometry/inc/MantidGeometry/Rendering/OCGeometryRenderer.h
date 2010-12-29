#ifndef OC_GEOMETRYRENDERER_H
#define OC_GEOMETRYRENDERER_H

#include "MantidKernel/System.h"
#include "MantidKernel/Logger.h"
class TopoDS_Shape;
namespace Mantid
{

  namespace Geometry
  {
    /*!
    \class OCGeometryRenderer
    \brief rendering geometry primitives of OpenCascade
    \author Srikanth Nagella
    \date July 2008
    \version 1.0

    This is an concrete class for rendering GtsSurface using opengl.

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
	  class IObjComponent;
	class DLLExport OCGeometryRenderer
    {
    private:

      static Kernel::Logger& PLog;           ///< The official logger
	  unsigned int           iDisplaylistId; ///< OpenGL display list id
	  bool                   boolDisplaylistCreated; ///< flag to store whether display list is created or not
	  void RenderTopoDS(TopoDS_Shape* ObjSurf);
    public:
		OCGeometryRenderer();       ///< Constructor
		~OCGeometryRenderer();      ///< Destructor
		void Render(TopoDS_Shape* ObjSurf);
		void Render(IObjComponent* ObjComp);
		void Initialize(TopoDS_Shape* ObjSurf);
		void Initialize(IObjComponent* ObjComp);
		void WriteVTK(TopoDS_Shape* out);
    };

  }   // NAMESPACE Geometry

}  // NAMESPACE Mantid

#endif
