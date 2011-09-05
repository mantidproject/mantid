#ifndef MANTID_DATAHANDLING_LOADINSTCOMPSINTOONESHAPE_H_
#define MANTID_DATAHANDLING_LOADINSTCOMPSINTOONESHAPE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadInstrument.h"

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------


namespace Mantid
{

  namespace DataHandling
  {
    /** @class LoadInstCompsIntoOneShape LoadInstCompsIntoOneShape.h DataHandling/LoadInstCompsIntoOneShape.h

    Loads instrument data from a XML instrument description file and adds it
    to a workspace.

    LoadInstCompsIntoOneShape is an algorithm and as such inherits
    from the Algorithm class and overrides the init() & exec()  methods.

    Required Properties:
    <UL>
    <LI> Workspace - The name of the workspace </LI>
    <LI> Filename - The name of the IDF file </LI>
    </UL>

    @author Anders Markvardsen, ISIS, RAL
    @date 4/9/2011

    Copyright &copy; 2007-2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport LoadInstCompsIntoOneShape
    {
    public:
      /// Default constructor
      /// @param angleConvertConst default equals 1 which means that angle=degree. See LoadInstrument.h for more
      LoadInstCompsIntoOneShape(const double angleConvertConst=1.0) : m_angleConvertConst(angleConvertConst) {}
      /// Destructor
      virtual ~LoadInstCompsIntoOneShape() {}

      /// return absolute position of point which is set relative to the
      /// coordinate system of the input component
      Kernel::V3D getAbsolutPositionInCompCoorSys(Geometry::ICompAssembly* comp, Kernel::V3D);

      /// Returns a translated and rotated <cuboid> element
      std::string translateRotateXMLcuboid(Geometry::ICompAssembly* comp, Poco::XML::Element* cuboidEle, 
                                    std::string& cuboidName);

      /// Takes as input a <type> element containing a <combine-components-into-one-shape>, and 
      /// adjust the <type> element by replacing its containing <component> elements with <cuboid>'s
      /// (note for now this will only work for <cuboid>'s and when necessary this can be extended).
      void adjust(Poco::XML::Element* pElem, std::map<std::string,bool>& isTypeAssembly, 
                  std::map<std::string,Poco::XML::Element*>& getTypeElement);
    private:

      /// Return a subelement of an XML element
      Poco::XML::Element* getShapeElement(Poco::XML::Element* pElem, const std::string& name);

      /// Get position coordinates from XML element
      Kernel::V3D parsePosition(Poco::XML::Element* pElem);

      /// see description in LoadInstrument.h 
      const double m_angleConvertConst;
    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADINSTCOMPSINTOONESHAPE_H_*/

