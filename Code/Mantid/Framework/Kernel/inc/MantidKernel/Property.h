#ifndef MANTID_KERNEL_PROPERTY_H_
#define MANTID_KERNEL_PROPERTY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/PropertyHistory.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/TimeSplitter.h"
#include <string>
#include <typeinfo>
#include <set>
#include "System.h"

namespace Mantid
{
namespace Kernel
{

/// Describes the direction (within an algorithm) of a Property. Used by WorkspaceProperty.
struct Direction
{
  /// Enum giving the possible directions
  enum Type
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

  /// Returns an enum representation of the input Direction string
  static int asEnum(const std::string& direction)
  {
    if( direction == "Input" ) return Direction::Input;
    else if( direction == "Output" ) return Direction::Output;
    else if( direction == "InOut" ) return Direction::InOut;
    else return Direction::None;
  }
  
};

/** Base class for properties. Allows access without reference to templated concrete type.

    @author Russell Taylor, Tessella Support Services plc
    @author Based on the Gaudi class of the same name (see http://proj-gaudi.web.cern.ch/proj-gaudi/)
    @date 13/11/2007

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
class DLLExport Property
{
public:
  /// 'Virtual copy constructor'
  virtual Property* clone() = 0;
  /// Virtual destructor
  virtual ~Property();

  // Getters
  const std::string& name() const;
  const std::string& documentation() const;
  const std::type_info* type_info() const;
  const std::string type() const;

  ///Overridden function that checks whether the property, if not overriden returns ""
  virtual std::string isValid() const;
  ///Overriden function that returns if property has the same value that it was initialised with, if applicable
  virtual bool isDefault() const = 0;
  ///Whether to save input values
  virtual bool remember() const;

  /**Sets the user level description of the property
   *  @param documentation :: The string that the user will see
   */
  void setDocumentation(const std::string& documentation);

  /// Returns the value of the property as a string
  virtual std::string value() const = 0;
  /// Set the value of the property via a string.  If the value is unacceptable the value is not changed but a string is returned
  virtual std::string setValue(const std::string&) = 0;
  /// Get the default value for the property which is the value the property was initialised with
  virtual std::string getDefault() const = 0;

  virtual std::set<std::string> allowedValues() const;

  virtual const PropertyHistory createHistory() const;

  /// returns the direction of the property
  unsigned int direction() const
  {
    return m_direction;
  }

  /// Add to this
  virtual Property& operator+=( Property const * rhs ) = 0;
  virtual void filterByTime(const Kernel::DateAndTime start, const Kernel::DateAndTime stop);
  virtual void splitByTime(Kernel::TimeSplitterType& splitter, std::vector< Property * > outputs) const;

  virtual int size() const;

  virtual std::string units() const;

  virtual void setUnits(std::string unit);

  virtual size_t getMemorySize() const
  { return sizeof(Property); }

  /** Just returns the property (*this) unless overridden
  *  @return a property with the value
  */
  virtual  Property& merge( Property * )
  { return *this; }

protected:
  /// Constructor
  Property( const std::string& name, const std::type_info& type, const unsigned int direction = Direction::Input);
  /// Copy constructor
  Property( const Property& right );

private:
  /// Private, unimplemented copy assignment operator
  Property& operator=( const Property& right );

  /// The name of the property
  const std::string m_name;
  /// Longer, optional description of property
  std::string m_documentation;
  /// The type of the property
  const std::type_info* m_typeinfo;
  /// Whether the property is used as input, output or both to an algorithm
  const unsigned int m_direction;
  /// Units of the property (optional)
  std::string m_units;

  /// Private default constructor
  Property();
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_PROPERTY_H_*/
