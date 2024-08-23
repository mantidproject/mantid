// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#ifndef Q_MOC_RUN
#include <memory>
#endif

#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace NeXus {
class File;
}

namespace Json {
class Value;
}
namespace std {
class typeinfo;
}

namespace Mantid {
namespace Types {
namespace Core {
class DateAndTime;
}
} // namespace Types
namespace Kernel {
//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class DataItem;
class IPropertySettings;
class PropertyHistory;
class SplittingInterval;

/// Describes the direction (within an algorithm) of a Property. Used by
/// WorkspaceProperty.
struct Direction {
  /// Enum giving the possible directions
  enum Type {
    Input,  ///< An input workspace
    Output, ///< An output workspace
    InOut,  ///< Both an input & output workspace
    None
  };

  /// Returns a text representation of the input Direction enum
  static const std::string asText(const unsigned int &direction) {
    switch (direction) {
    case Input:
      return "Input";
    case Output:
      return "Output";
    case InOut:
      return "InOut";
    default:
      return "N/A";
    }
  }

  /// Returns an enum representation of the input Direction string
  static int asEnum(const std::string &direction) {
    if (direction == "Input")
      return Direction::Input;
    else if (direction == "Output")
      return Direction::Output;
    else if (direction == "InOut")
      return Direction::InOut;
    else
      return Direction::None;
  }
};

/** Base class for properties. Allows access without reference to templated
   concrete type.

    @author Russell Taylor, Tessella Support Services plc
    @author Based on the Gaudi class of the same name (see
   http://proj-gaudi.web.cern.ch/proj-gaudi/)
    @date 13/11/2007
*/
class MANTID_KERNEL_DLL Property {
public:
  /// 'Virtual copy constructor'
  virtual Property *clone() const = 0;
  /// Virtual destructor
  virtual ~Property();

  // Getters
  const std::string &name() const;
  const std::string &documentation() const;
  const std::type_info *type_info() const;
  const std::string type() const;

  /// Overridden function that checks whether the property, if not overriden
  /// returns ""
  virtual std::string isValid() const;

  /// Set the PropertySettings object
  void setSettings(std::unique_ptr<IPropertySettings> settings);
  /** @return the PropertySettings for this property */
  IPropertySettings *getSettings();
  /** Deletes the PropertySettings object contained */
  void clearSettings();

  /// Overriden function that returns if property has the same value that it was
  /// initialised with, if applicable
  virtual bool isDefault() const = 0;
  /// Whether to save input values
  bool remember() const;
  void setRemember(bool);

  void setDocumentation(const std::string &documentation);

  virtual void saveProperty(::NeXus::File * /*file*/) {
    throw std::invalid_argument("Property::saveProperty - Cannot save '" + this->name() +
                                "', property type not implemented.");
  }
  /// Returns the value of the property as a string
  virtual std::string value() const = 0;
  /// Returns the value of the property as a pretty printed string
  virtual std::string valueAsPrettyStr(const size_t maxLength = 0, const bool collapseLists = true) const;
  /// Returns the value of the property as a Json::Value
  virtual Json::Value valueAsJson() const = 0;
  /// Whether the string returned by value() can be used for serialization.
  virtual bool isValueSerializable() const { return true; }
  /// Set the value of the property via a string.  If the value is unacceptable
  /// the value is not changed but a string is returned
  virtual std::string setValue(const std::string &) = 0;
  /// Set the value of the property via a Json object.  If the value is
  /// unacceptable the value is not changed but a string is returned
  /// A const char * can be implicitly converted to both Json::Value
  /// and std::string so using simple setValue for both functions
  /// causes an abiguity error
  virtual std::string setValueFromJson(const Json::Value &) = 0;
  /// Set the value of the property via a reference to another property.
  virtual std::string setValueFromProperty(const Property &right) = 0;
  /// Set the value of the property via a DataItem pointer.  If the value is
  /// unacceptable the value is not changed but a string is returned
  virtual std::string setDataItem(const std::shared_ptr<DataItem> &) = 0;
  /// Get the default value for the property which is the value the property was
  /// initialised with
  virtual std::string getDefault() const = 0;

  /** Is Multiple Selection Allowed
   *  @return true if multiple selection is allowed
   */
  virtual bool isMultipleSelectionAllowed() { return false; }

  virtual std::vector<std::string> allowedValues() const;

  virtual const PropertyHistory createHistory() const;

  /// Create a temporary value for this property
  void createTemporaryValue();
  /// Property is using a temporary value for this property
  bool hasTemporaryValue() const;

  /// returns the direction of the property
  unsigned int direction() const { return m_direction; }

  /// Add to this
  virtual Property &operator+=(Property const *rhs) = 0;

  virtual int size() const;

  virtual const std::string &units() const;

  virtual void setUnits(const std::string &unit);

  virtual size_t getMemorySize() const { return sizeof(Property); }

  /** Just returns the property (*this) unless overridden
   *  @return a property with the value
   */
  virtual Property &merge(Property *) { return *this; }

  /// Set the group this property belongs to
  void setGroup(const std::string &group) { m_group = group; }

  /// @return the group this property belongs to
  const std::string &getGroup() { return m_group; }

  bool autoTrim() const;
  void setAutoTrim(const bool &setting);

  bool disableReplaceWSButton() const;
  void setDisableReplaceWSButton(const bool &disable);

protected:
  /// Constructor
  Property(std::string name, const std::type_info &type, const unsigned int &direction = Direction::Input);
  /// Copy constructor
  Property(const Property &right);
  /// The name of the property
  std::string m_name;

private:
  /// Private, unimplemented copy assignment operator
  Property &operator=(const Property &right);

  /// Longer, optional description of property
  std::string m_documentation;
  /// The type of the property
  const std::type_info *m_typeinfo;
  /// Whether the property is used as input, output or both to an algorithm
  const unsigned int m_direction;
  /// Units of the property (optional)
  std::string m_units;

  /// Property settings (enabled/visible)
  std::unique_ptr<IPropertySettings> m_settings;

  /// Name of the "group" of this property, for grouping in the GUI. Default ""
  std::string m_group;

  /// Private default constructor
  Property();

  /// Flag whether to save input values
  bool m_remember;

  /// Flag to determine if string inputs to the property should be automatically
  /// trimmed of whitespace
  bool m_autotrim;

  /// Flag to disable the generation of the "Replace Workspace" button on the OutputWorkspace property
  bool m_disableReplaceWSButton;
};

/// Compares this to another property for equality
MANTID_KERNEL_DLL bool operator==(const Mantid::Kernel::Property &lhs, const Mantid::Kernel::Property &rhs);
/// Compares this to another property for inequality
MANTID_KERNEL_DLL bool operator!=(const Mantid::Kernel::Property &lhs, const Mantid::Kernel::Property &rhs);

/// Return the name corresponding to the mangled string given by typeid
MANTID_KERNEL_DLL std::string getUnmangledTypeName(const std::type_info &type);

} // namespace Kernel
} // namespace Mantid
