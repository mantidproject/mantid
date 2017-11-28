#ifndef STRUCTUREDGEOMETRYHANDLER_H
#define STRUCTUREDGEOMETRYHANDLER_H

#ifndef Q_MOC_RUN
#include <boost/weak_ptr.hpp>
#endif
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Instrument/StructuredDetector.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidKernel/Logger.h"

namespace Mantid {
namespace Geometry {
/**
\class StructuredGeometryHandler
\brief Handler for StructuredDetector geometry objects
\author Lamar Moore
\date March 2016
\version 1.0

This class supports drawing StructuredDetector.

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
class ObjComponent;
class Object;
class MANTID_GEOMETRY_DLL StructuredGeometryHandler : public GeometryHandler {
private:
  static Kernel::Logger &PLog; ///< The official logger

  boost::shared_ptr<GeometryHandler> clone() const override;

  /// The StructuredDetector object being drawn.
  StructuredDetector *m_Det;

public:
  StructuredGeometryHandler(StructuredDetector *comp);
  StructuredGeometryHandler();

  StructuredGeometryHandler *createInstance(
      IObjComponent *) override; ///< Create an instance of concrete geometry
                                 /// handler for ObjComponent
  StructuredGeometryHandler *createInstance(boost::shared_ptr<Object>)
      override; ///< Create an instance of concrete geometry handler for Object
  GeometryHandler *createInstance(Object *)
      override; ///< Create an instance of concrete geometry handler for Object
  void Triangulate() override; ///< Triangulate the Object
  void Render() override;      ///< Render Object or ObjComponent
  void Initialize()
      override; ///< Prepare/Initialize Object/ObjComponent to be rendered
                /// Returns true if the shape can be triangulated
  bool canTriangulate() override { return false; }
};

} // NAMESPACE Geometry

} // NAMESPACE Mantid

#endif // STRUCTUREDGEOMETRYHANDLER_H