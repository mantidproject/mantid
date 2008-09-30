#ifndef GTS_GEOMETRYHANDLER_H
#define GTS_GEOMETRYHANDLER_H

#include "MantidKernel/System.h"
#include "boost/shared_ptr.hpp"

namespace Mantid
{

	namespace Geometry
	{
		/*!
		\class GtsGeometryHandler
		\brief Place holder for GTS library geometry triangulation and rendering.
		\author Srikanth Nagella
		\date July 2008
		\version 1.0

		This is an implementation class for handling geometry using GTS(GNU Triangulation Surface).

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
		class GeometryHandler;
		class GtsGeometryRenderer;
		class GtsGeometryGenerator;
		class ObjComponent;
		class Object;
		class DLLExport GtsGeometryHandler: public GeometryHandler
		{
		private:
			static Kernel::Logger& PLog;           ///< The official logger
			GtsGeometryRenderer* Renderer;         ///< Geometry renderer variable used for rendering Object/ObjComponent
			GtsGeometryGenerator* Triangulator;    ///< Geometry generator to triangulate Object
		public:
			GtsGeometryHandler(ObjComponent* obj); ///< Constructor
			GtsGeometryHandler(boost::shared_ptr<Object>       obj); ///< Constructor
			GtsGeometryHandler(Object* obj); ///< Constructor
			~GtsGeometryHandler(); ///< Destructor
			GeometryHandler* createInstance(ObjComponent *comp);
			GeometryHandler* createInstance(boost::shared_ptr<Object> obj);
			void Triangulate();
			void Render();
			void Initialize();
		};

	}   // NAMESPACE Geometry

}  // NAMESPACE Mantid

#endif
