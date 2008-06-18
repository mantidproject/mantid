#ifndef MANTID_KERNEL_PROPERTY_H_
#define MANTID_KERNEL_PROPERTY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <string>
#include <typeinfo>
#include <set>
#include "System.h"

namespace Mantid
{
namespace Kernel
{
/** @class Property Property.h Kernel/Property.h

    Base class for properties. Allows access without reference to templated concrete type.

    @author Russell Taylor, Tessella Support Services plc
    @author Based on the Gaudi class of the same name (see http://proj-gaudi.web.cern.ch/proj-gaudi/)
    @date 13/11/2007
    
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
class DLLExport Property
{
public:
  /// Virtual destructor
	virtual ~Property();
	
	// Getters
	const std::string& name() const;
	const std::string& documentation() const;
	const std::type_info* type_info() const;
	const std::string type() const;
	virtual const bool isValid() const;
	const bool isDefault() const;
	
	// Setter
	void setDocumentation( const std::string& documentation );
	
	/// Returns the value of the property as a string
	virtual std::string value() const = 0;
	/// Set the value of the property via a string
	virtual bool setValue( const std::string& value ) = 0;
	
	virtual const std::set<std::string> allowedValues() const;
	
protected:
  /// Constructor
  Property( const std::string& name, const std::type_info& type );
	/// Copy constructor
  Property( const Property& right );
  /// Copy assignment operator
  virtual Property& operator=( const Property& right );
  /// Whether the property has been changed since initialisation
  bool m_isDefault;
  
private:
  /// The name of the property
  const std::string m_name;
  /// Longer, optional description of property
  std::string m_documentation;
  /// The type of the property
  const std::type_info* m_typeinfo;
  
  /// Private default constructor
  Property();
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_PROPERTY_H_*/
