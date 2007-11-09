#ifndef MANTID_KERNEL_IPROPERTY_H_
#define MANTID_KERNEL_IPROPERTY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "INamedInterface.h"

namespace Mantid
{
namespace Kernel
{
// Declaration of the interface ID ( interface id, major version, minor version)
// static const InterfaceID IID_IProperty("IProperty", 2 , 0); 

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

    /// Set the property by property
// RJT: No Property class yet
//    virtual StatusCode setProperty( const Property& p // Reference to the input property
//                                  ) = 0;
    
    /** Set a property which has no value
     * 
     *  @param s A string containing the property
     * 
     *  @return A StatusCode object indicating whether the operation was successful
     */
    virtual StatusCode setProperty( const std::string& s ) = 0;
    
    /** Set a property and its value
     * 
     *  @param n The property's name
     *  @param v The property's value
     * 
     *  @return A StatusCode object indicating whether the operation was successful
     */
    virtual StatusCode setProperty( const std::string& n, const std::string& v ) = 0;
    
    /// Get the property by property
//    virtual StatusCode getProperty( Property* p       // Pointer to property to be set
//                                  ) const = 0;
    /// Get the property by name
//    virtual const Property& getProperty( const std::string& name  // Property name
//                                  ) const = 0;
    /// Get the property by std::string
    
    /** Get the value of a named property
     * 
     *  @param n The name of the property
     *  @param v Returns a reference to the value
     * 
     *  @return A StatusCode object indicating whether the operation was successful
     */
    virtual StatusCode getProperty( const std::string& n, std::string& v ) const = 0;
    
    /// Get list of properties
//    virtual const std::vector<Property*>& getProperties( ) const = 0;
        
  };

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_IPROPERTY_H_*/
