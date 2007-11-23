#ifndef MANTID_KERNEL_IPROPERTY_H_
#define MANTID_KERNEL_IPROPERTY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/INamedInterface.h"
#include <vector>

namespace Mantid
{
namespace Kernel
{
// Declaration of the interface ID ( interface id, major version, minor version)
// static const InterfaceID IID_IProperty("IProperty", 2 , 0); 

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class Property;

/** @class IProperty IProperty.h Kernel/IProperty.h

    IProperty is the basic interface for all components which have 
    properties that can be set or get.

    @author Russell Taylor, Tessella Support Services plc
    @author Based on the Gaudi class of the same name (see http://proj-gaudi.web.cern.ch/proj-gaudi/)
    @date 19/09/2007
    
    Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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
  class DLLExport IProperty : public INamedInterface 
  {
  public:
    /// Retrieve interface ID
//    static const InterfaceID& interfaceID() { return IID_IProperty; }

    /** Set a property's value
     * 
     *  @param name The property's name
     *  @param value The property's value
     */
    virtual void setProperty( const std::string& name, const std::string& value ) = 0;
    
    /** Checks whether the named property already exists.
     * 
     *  @param name The name of the property
     *  @return True if the property exists
     */
    virtual bool existsProperty( const std::string& name ) const = 0;
    
    /** Get the value of a property as a string
     * 
     *  @param name The name of the property
     *  @return The value of the named property
     */
    virtual std::string getPropertyValue( const std::string &name ) const = 0;

    /** Get a property by name
     * 
     *  @param name The name of the property
     *  @return A pointer to the named property
     */
    virtual Property* getProperty( std::string name ) const = 0;
        
    /** Get the list of properties
     * 
     *  @return A vector holding pointers to the list of properties
     */
    virtual const std::vector< Property* >& getProperties( ) const = 0;
        
  };

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_IPROPERTY_H_*/
