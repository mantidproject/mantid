#ifndef MANTID_KERNEL_PROPERTY_H_
#define MANTID_KERNEL_PROPERTY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/PropertyHistory.h"
#include <string>
#include <typeinfo>
#include <vector>
#include "System.h"

namespace Mantid
{
namespace Kernel
{

/// Describes the direction (within an algorithm) of a Property. Used by WorkspaceProperty.
struct Direction
{
  /// Enum giving the possible directions
  enum
  {
    Input,    ///< An input workspace
    Output,   ///< An output workspace
    InOut,     ///< Both an input & output workspace
    None
  };

  /// Returns a text representation of the input Direction enum
  static const std::string asText(const unsigned int& direction)
  {
    switch (direction)
    {
    case Input:  return "Input";
    case Output: return "Output";
    case InOut:  return "InOut";
    default:     return "N/A";
    }
  }
};

/** @class Property Property.h Kernel/Property.h

    Base class for properties. Allows access without reference to templated concrete type.

    @author Russell Taylor, Tessella Support Services plc
    @author Based on the Gaudi class of the same name (see http://proj-gaudi.web.cern.ch/proj-gaudi/)
    @date 13/11/2007

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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
	virtual const std::string getValidatorType() const;
	const bool isDefault() const;

	// Setter
	void setDocumentation( const std::string& documentation );

	/// Returns the value of the property as a string
	virtual std::string value() const = 0;
	/// Set the value of the property via a string
	virtual bool setValue( const std::string& value ) = 0;

	virtual const std::vector<std::string> allowedValues() const;

	virtual const PropertyHistory createHistory() const;

    /// returns the direction of the property
    const unsigned int direction() const
    {
      return m_direction;
    }

protected:
  /// Constructor
  Property( const std::string& name, const std::type_info& type, const unsigned int direction = Direction::Input);
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
  /// Whether the property is used as input, output or both to an algorithm
  const unsigned int m_direction;

  /// Private default constructor
  Property();
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_PROPERTY_H_*/
