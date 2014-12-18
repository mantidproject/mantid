#ifndef CACHE_GEOMETRYRENDERER_H
#define CACHE_GEOMETRYRENDERER_H

#include "MantidGeometry/DllConfig.h"

namespace Mantid {
namespace Kernel {
class V3D;
}
namespace Geometry {
class IObjComponent;
/**
   \class CacheGeometryRenderer
   \brief rendering geometry using opengl from the geometry cache.
   \author Srikanth Nagella
   \date Jan 2009
   \version 1.0

   This is an concrete class for rendering cached geometry using opengl.

   Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

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

   File change history is stored at: <https://github.com/mantidproject/mantid>
*/
class MANTID_GEOMETRY_DLL CacheGeometryRenderer {
public:
  CacheGeometryRenderer();  ///< Constructor
  ~CacheGeometryRenderer(); ///< Destructor
  /// Render using an object component
  void Render(IObjComponent *ObjComp) const;
  /// Render using triangulation information
  void Render(int noPts, int noFaces, double *points, int *faces) const;
  /// Initialize using triangulation information
  void Initialize(int noPts, int noFaces, double *points, int *faces) const;
  /// Initialize using an object component
  void Initialize(IObjComponent *ObjComp);
};

} // NAMESPACE Geometry

} // NAMESPACE Mantid

#endif
