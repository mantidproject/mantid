#ifndef MANTID_GEOMETRY_SHAPEFACTORY_H_
#define MANTID_GEOMETRY_SHAPEFACTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidGeometry/V3D.h"
#include <boost/shared_ptr.hpp>
#include <map>

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
/// @cond Exclude from doxygen documentation
namespace Poco {
namespace XML {
	class Element;
}}
/// @endcond

namespace Mantid
{	
  namespace Kernel
  {
    class Logger;
  }

  namespace Geometry
  {
    class Surface;
    class Object;
    
    /**

    Class originally intended to be used with the DataHandling 'LoadInstrument' algorithm.
    In that algorithm it is used for creating shared pointers to the geometric shapes 
    described in the XML instrument definition file.

    This class is now also use elsewhere, and in addition to create geometric shapes
    from an DOM-element-node pointing to a \<type> element with shape information, shapes
    can also be created directly from a XML shape string. 

    @author Anders Markvardsen, ISIS, RAL
    @date 6/8/2008

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
      ShapeFactory();
      /// Destructor
      ~ShapeFactory() {}

      boost::shared_ptr<Object> createShape(Poco::XML::Element* pElem);
      boost::shared_ptr<Object> createShape(std::string shapeXML);
      
    private:
      std::string parseSphere(Poco::XML::Element* pElem, std::map<int, Surface*>& prim, int& l_id);
      std::string parseInfinitePlane(Poco::XML::Element* pElem, std::map<int, Surface*>& prim, int& l_id);
      std::string parseInfiniteCylinder(Poco::XML::Element* pElem, std::map<int, Surface*>& prim, int& l_id);
      std::string parseCylinder(Poco::XML::Element* pElem, std::map<int, Surface*>& prim, int& l_id);
      std::string parseSegmentedCylinder(Poco::XML::Element* pElem, std::map<int, Surface*>& prim, int& l_id);
      std::string parseCuboid(Poco::XML::Element* pElem, std::map<int, Surface*>& prim, int& l_id);
      std::string parseInfiniteCone(Poco::XML::Element* pElem, std::map<int, Surface*>& prim, int& l_id);
      std::string parseCone(Poco::XML::Element* pElem, std::map<int, Surface*>& prim, int& l_id);
      std::string parseHexahedron(Poco::XML::Element* pElem, std::map<int, Surface*>& prim, int& l_id);
      std::string parseTorus(Poco::XML::Element* pElem, std::map<int, Surface*>& prim, int& l_id);
      std::string parseSliceOfCylinderRing(Poco::XML::Element* pElem, std::map<int, Surface*>& prim, int& l_id);

      Poco::XML::Element* getShapeElement(Poco::XML::Element* pElem, const std::string& name);
      double getDoubleAttribute(Poco::XML::Element* pElem, const std::string& name);
      V3D parsePosition(Poco::XML::Element* pElem);	
      void createGeometryHandler(Poco::XML::Element*,boost::shared_ptr<Object>);

      /// static reference to the logger class
      static Kernel::Logger& g_log;
    };

  } // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_SHAPEFACTORY_H_*/

