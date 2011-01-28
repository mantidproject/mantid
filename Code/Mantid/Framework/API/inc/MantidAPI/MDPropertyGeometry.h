#ifndef MDPROPERTY_GEOMETRY_DESCRIPTION_H
#define MDPROPERTY_GEOMETRY_DESCRIPTION_H


//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/PropertyWithValue.h"
#include "MantidAPI/AnalysisDataService.h"
#include <boost/shared_ptr.hpp>
#include "MantidKernel/Logger.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"


#include <iostream>
#include <fstream>
#include <string>

namespace Mantid
{
  namespace API
  {
    /** The property class driver for MD workspaces. Inherits from PropertyWithValue, with the value being
    a pointer to the workspace type given to the WorkspaceProperty constructor. This kind
    of property also holds the name of the workspace (as used by the AnalysisDataService)

    it is always input property for algorithms though may be need to be output? (for visualisation)
    
    The body of the class copypasted from workspace property but some important methods are overloaded as they are not applicable
    to this kind of porperties

    The pointers to the workspaces are fetched from the ADS when the properties are validated
    (i.e. when the PropertyManager::validateProperties() method calls isValid() ).
    Pointers to output workspaces are also fetched, if they exist, and can then be used within
    an algorithm. (An example of when this might be useful is if the user wants to write the
    output into the same workspace as is used for input - this avoids creating a new workspace
    and the overwriting the old one at the end.)

    @author Alex Buts, ISIS RAL
    @date 29/10/2010

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */	
    
    class DLLExport MDPropertyGeometry :  public Geometry::MDGeometryDescription, public Kernel::PropertyWithValue<std::string>
    {
    public:
      /** Constructor.
      *  Sets the property and workspace names but initialises the workspace pointer to null.
      *  @param name :: The name to assign to the property
      *  @param Value : property value expressed as a string
      *  @param direction :: Whether this is a Direction::Input, Direction::Output or In-Out property
      *  @param validator :: The (optional) validator to use for this property
      *  @throw std::out_of_range if the direction argument is not a member of the Direction enum (i.e. 0-2)
      */
      MDPropertyGeometry( const std::string &name, const std::string &Value="", const unsigned int direction=Kernel::Direction::Input,
        Kernel::IValidator<std::string > *validator = new Kernel::NullValidator<std::string> ) :
        MDGeometryDescription(),
        Kernel::PropertyWithValue<std::string>(name, Value, validator, direction)
      {
          // fill in binary implementation of the property.
          this->fromXMLstring(this->value());
      }
      MDPropertyGeometry( const std::string &name, const Geometry::MDGeometryDescription &Geom, const unsigned int direction=Kernel::Direction::Input,
        Kernel::IValidator<Geometry::MDGeometryDescription> *validator = new Kernel::NullValidator<Geometry::MDGeometryDescription> ) :
        Geometry::MDGeometryDescription(Geom),
        Kernel::PropertyWithValue<std::string>(name, "", new Kernel::NullValidator<std::string> , direction)
      {
          // fill in symbol representation of the property. 
         this->Kernel::PropertyWithValue<std::string>::setValue(this->toXMLstring());
      }
     //MDGeometryDescription const *getpMDGeometryDescription(void){return 
     // overloads IO operations for property for the lexical casts to work 
      friend std::ostream &operator<<(std::ostream &stream, const MDPropertyGeometry &obj){
          return (stream<<obj.toXMLstring());
      }
      friend std::istream& operator>>(std::istream& in,MDPropertyGeometry &obj){         
          std::string buf;
          while(!in.eof()){
            in>>buf;
          }
          obj.fromXMLstring(buf);     
          return in;
       }
      /// set geometry description on the basis of an existing geometry; Should return XML description of the property (set as value of this porpety) or empty string if this property is unacceptable
      virtual std::string setValue(const Geometry::MDGeometry &origin);
     /// set geometry description on the basis of an XML string; returns what usually property with value string returns
      virtual std::string setValue(const std::string &XMLstring);
      /// Virtual destructor
      virtual ~MDPropertyGeometry()
      {
      }
    private:
 
  
    };


   
  
  } // namespace API
} // namespace Mantid

#endif
