#ifndef MANTID_DATAHANDLING_SHAPEFACTORY_H_
#define MANTID_DATAHANDLING_SHAPEFACTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include <set>
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidGeometry/Surfaces/Plane.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/Cone.h"
#include "MantidGeometry/Surfaces/Torus.h"
#include "MantidGeometry/Objects/Object.h"

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
/// @cond Exclude from doxygen documentation
namespace Poco {
namespace XML {
	class Element;
}}
/// @endcond

namespace Mantid
{	
  namespace Geometry
  {
    /** @class ShapeFactory ShapeFactory.h DataHandling/ShapeFactory.h

    Class originally intended to be used with the DataHandling 'LoadInstrument' algorithm.
    In that algorithm it is used for creating shared pointers to the geometric shapes 
    described in the XML instrument definition file.

    This class is now also use elsewhere, and in addition to create geometric shapes
    from an DOM-element-node pointing to a \<type> element with shape information, shapes
    can also be created directly from a XML shape string. 

    @author Anders Markvardsen, ISIS, RAL
    @date 6/8/2008

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratories

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
    class DLLExport ShapeFactory
    {
    public:
      /// Default constructor
      ShapeFactory();

      /// Destructor
      ~ShapeFactory() {}

      /// Creates a geometric object from a DOM-element-node pointing to a \<type> element
      /// containing shape information. If no shape information an empty Object is returned
      boost::shared_ptr<Object> createShape(Poco::XML::Element* pElem);

      /// Creates a geometric object directly from a XML shape string
      boost::shared_ptr<Object> createShape(std::string &shapeXML);
    private:

      /// Parse XML 'sphere' element
      std::string parseSphere(Poco::XML::Element* pElem, std::map<int, Surface*>& prim, int& l_id);

      /// Parse XML 'infinite-plane' element
      std::string parseInfinitePlane(Poco::XML::Element* pElem, std::map<int, Surface*>& prim, int& l_id);

      /// Parse XML 'infinite-cylinder' element
      std::string parseInfiniteCylinder(Poco::XML::Element* pElem, std::map<int, Surface*>& prim, int& l_id);

      /// Parse XML 'cylinder' element
      std::string parseCylinder(Poco::XML::Element* pElem, std::map<int, Surface*>& prim, int& l_id);

      /// Parse XML 'cuboid' element
      std::string parseCuboid(Poco::XML::Element* pElem, std::map<int, Surface*>& prim, int& l_id);

      /// Parse XML 'infinite-cone' element
      std::string parseInfiniteCone(Poco::XML::Element* pElem, std::map<int, Surface*>& prim, int& l_id);

      /// Parse XML 'cone' element
      std::string parseCone(Poco::XML::Element* pElem, std::map<int, Surface*>& prim, int& l_id);

      /// Parse XML 'hexahedron' element
      std::string parseHexahedron(Poco::XML::Element* pElem, std::map<int, Surface*>& prim, int& l_id);

      /// Parse XML 'torus' element
      std::string parseTorus(Poco::XML::Element* pElem, std::map<int, Surface*>& prim, int& l_id);

      /// Parse XML 'slice-of-cylinder-ring' element
      std::string parseSliceOfCylinderRing(Poco::XML::Element* pElem, std::map<int, Surface*>& prim, int& l_id);

      /// Return a subelement of an XML element, but also checks that there exist exactly one entry of this subelement
      Poco::XML::Element* getShapeElement(Poco::XML::Element* pElem, const std::string& name);

      /// Return value of attribute to XML element. It is an extension of poco's getAttribute method
      double getDoubleAttribute(Poco::XML::Element* pElem, const std::string& name);

      /// Parse any XML element containing position attributes and return as V3D
      V3D parsePosition(Poco::XML::Element* pElem);
	
      /// create a special geometry handler for the known finite primitives
      void createGeometryHandler(Poco::XML::Element*,boost::shared_ptr<Object>);

      ///static reference to the logger class
      static Kernel::Logger& g_log;

    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_SHAPEFACTORY_H_*/

