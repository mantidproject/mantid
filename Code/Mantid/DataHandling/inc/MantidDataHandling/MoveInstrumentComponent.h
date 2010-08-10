#ifndef MANTID_DATAHANDLING_MOVEINSTRUMENTCOMPONENT_H_
#define MANTID_DATAHANDLING_MOVEINSTRUMENTCOMPONENT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Instrument/Component.h"

namespace Mantid
{
	
  namespace DataHandling
  {
    /** @class MoveInstrumentComponent MoveInstrumentComponent.h DataHandling/MoveInstrumentComponent.h

    Moves an instrument component to a new position by setting corresponding parameter in ParameterMap.

    Required Properties:
    <UL>
    <LI> Workspace - The workspace to which the change will apply </LI>
    <LI> ComponentName - The name of the component which will be moved </LI>
    <LI> DetectorID - The detector id of the component to move. Either ComponentName or DetectorID 
         can be used to identify the component. If both are given the DetectorID will bw used.
    </LI>
    <LI> X - New x coordinate of the component or the shift along x axis depending on the value of RelativePosition property.</LI>
    <LI> Y - New y coordinate of the component or the shift along y axis depending on the value of RelativePosition property.</LI>
    <LI> Z - New z coordinate of the component or the shift along z axis depending on the value of RelativePosition property.</LI>
    <LI> RelativePosition - Boolean. If false (X,Y,Z) is the new absolute position of the component, if true (X,Y,Z) is the 
    shift and NewPos = OldPos + (X,Y,Z). The default value is true.</LI>
    </UL>

    @author Roman Tolchenov, Tessella Support Services plc
    @date 21/01/2009

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>. 
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport MoveInstrumentComponent : public API::Algorithm
    {
    public:
      /// Default constructor
      MoveInstrumentComponent();

      /// Destructor
      ~MoveInstrumentComponent() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "MoveInstrumentComponent";};
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;};
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling\\Instrument";}

    private:

      /// Overwrites Algorithm method.
      void init();

      /// Overwrites Algorithm method
      void exec();

      /// Find component by detector id
      boost::shared_ptr<Geometry::IComponent> findByID(boost::shared_ptr<Geometry::IComponent> instr,int id);

      /// Find component by name
      boost::shared_ptr<Geometry::IComponent> findByName(boost::shared_ptr<Geometry::IComponent> instr,const std::string& CName);

    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_MOVEINSTRUMENTCOMPONENT_H_*/
