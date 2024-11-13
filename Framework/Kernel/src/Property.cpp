// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/Property.h"

#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/IPropertySettings.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/PropertyHistory.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <unordered_map>

namespace Mantid {
namespace Kernel {

/** Constructor
 *  @param name :: The name of the property
 *  @param type :: The type of the property
 *  @param direction :: Whether this is a Direction::Input, Direction::Output or
 * Direction::InOut (Input & Output) property
 * @throws std::invalid_argument if the name is empty
 */
Property::Property(std::string name, const std::type_info &type, const unsigned int &direction)
    : m_name(std::move(name)), m_documentation(""), m_typeinfo(&type), m_direction(direction), m_units(""), m_group(""),
      m_remember(true), m_autotrim(true), m_disableReplaceWSButton(false) {
  if (m_name.empty()) {
    throw std::invalid_argument("An empty property name is not permitted");
  }

  // Make sure a random int hasn't been passed in for the direction
  // Property & PropertyWithValue destructors will be called in this case
  if (m_direction > 2)
    throw std::out_of_range("direction should be a member of the Direction enum");
}

/// Copy constructor
Property::Property(const Property &right)
    : m_name(right.m_name), m_documentation(right.m_documentation), m_typeinfo(right.m_typeinfo),
      m_direction(right.m_direction), m_units(right.m_units), m_group(right.m_group), m_remember(right.m_remember),
      m_autotrim(right.m_autotrim), m_disableReplaceWSButton(right.m_disableReplaceWSButton) {
  if (m_name.empty()) {
    throw std::invalid_argument("An empty property name is not permitted");
  }
  if (right.m_settings)
    m_settings.reset(right.m_settings->clone());
}

/// Virtual destructor
Property::~Property() = default;

/** Get the property's name
 *  @return The name of the property
 */
const std::string &Property::name() const { return m_name; }

/** Get the property's documentation string
 *  @return The documentation string
 */
const std::string &Property::documentation() const { return m_documentation; }

/** Get the property type_info
 *  @return The type of the property
 */
const std::type_info *Property::type_info() const { return m_typeinfo; }

/** Returns the type of the property as a string.
 *  Note that this is implementation dependent.
 *  @return The property type
 */
const std::string Property::type() const { return Mantid::Kernel::getUnmangledTypeName(*m_typeinfo); }

/** Overridden functions checks whether the property has a valid value.
 *
 *  @return empty string ""
 */
std::string Property::isValid() const {
  // the no error condition
  return "";
}

/**
 * Set the PropertySettings determining when this property is visible/enabled.
 * Takes ownership of the given object
 * @param settings A pointer to an object specifying the settings type
 */
void Property::setSettings(std::unique_ptr<IPropertySettings> settings) { m_settings = std::move(settings); }

/**
 *
 * @return the PropertySettings for this property
 */
IPropertySettings *Property::getSettings() { return m_settings.get(); }

/**
 * Deletes the PropertySettings object contained
 */
void Property::clearSettings() { m_settings.reset(nullptr); }

/**
 * Whether to remember this property input
 * @return whether to remember this property's input
 */
bool Property::remember() const { return m_remember; }

/**
 * Set wheter to remeber this property input
 * @param remember :: true to remember
 */
void Property::setRemember(bool remember) { m_remember = remember; }

/**
 * Returns the value as a pretty printed string
 * The default implementation just returns the value with the size limit applied
 * @param maxLength :: The Max length of the returned string
 * @param collapseLists :: Whether to collapse 1,2,3 into 1-3
 */
std::string Property::valueAsPrettyStr(const size_t maxLength, const bool collapseLists) const {
  UNUSED_ARG(collapseLists);
  return Strings::shorten(value(), maxLength);
}

/** Sets the user level description of the property.
 *  In addition, if the brief documentation string is empty it will be set to
 *  the portion of the provided string up to the first period
 *  (or the entire string if no period is found).
 *  @param documentation The string containing the descriptive comment
 */
void Property::setDocumentation(const std::string &documentation) { m_documentation = documentation; }

/** Returns the set of valid values for this property, if such a set exists.
 *  If not, it returns an empty set.
 * @return the set of valid values for this property or an empty set
 */
std::vector<std::string> Property::allowedValues() const { return std::vector<std::string>(); }

/// Create a PropertyHistory object representing the current state of the
/// Property.
const PropertyHistory Property::createHistory() const { return PropertyHistory(this); }

/** Creates a temporary property value based on the memory address of
 *  the property.
 */
void Property::createTemporaryValue() {
  std::ostringstream os;
  os << "__TMP" << this;
  this->setValue(os.str());
}

/** Checks if the property value is a temporary one based on the memory address
 * of
 *  the property.
 */
bool Property::hasTemporaryValue() const {
  std::ostringstream os;
  os << "__TMP" << this;
  return (os.str() == this->value());
}

//-------------------------------------------------------------------------------------------------
/** Return the size of this property.
 * Single-Value properties return 1.
 * TimeSeriesProperties return the # of entries.
 * @return the size of the property
 */
int Property::size() const { return 1; }

//-------------------------------------------------------------------------------------------------
/** Returns the units of the property, if any, as a string.
 * Units are optional, and will return empty string if they have
 * not been set before.
 * @return the property's units
 */
const std::string &Property::units() const { return m_units; }

//-------------------------------------------------------------------------------------------------
/** Sets the units of the property, as a string. This is optional.
 *
 * @param unit :: string to set for the units.
 */
void Property::setUnits(const std::string &unit) { m_units = unit; }

} // namespace Kernel

//-------------------------- Utility function for class name lookup
//-----------------------------

// MG 16/07/09: Some forward declarations.I need this so
// that the typeid function in getUnmangledTypeName knows about them
// This way I don't need to actually include the headers and I don't
// introduce unwanted dependencies
namespace API {
class Workspace;
class WorkspaceGroup;
class MatrixWorkspace;
class ITableWorkspace;
class IMDEventWorkspace;
class IMDWorkspace;
class IEventWorkspace;
class IPeaksWorkspace;
class IMDHistoWorkspace;
class IFunction;
class IAlgorithm;
} // namespace API
namespace DataObjects {
class EventWorkspace;
class PeaksWorkspace;
class LeanElasticPeaksWorkspace;
class GroupingWorkspace;
class OffsetsWorkspace;
class MaskWorkspace;
class SpecialWorkspace2D;
class Workspace2D;
class TableWorkspace;
class SpecialWorkspace2D;
class SplittersWorkspace;
} // namespace DataObjects

namespace Kernel {
class PropertyManager;

/**
 * @param lhs Thing on the left
 * @param rhs Thing on the right
 * @return true if they are equal
 */
bool operator==(const Property &lhs, const Property &rhs) {
  if (lhs.name() != rhs.name())
    return false;
  if (lhs.type() != rhs.type())
    return false;

  // check for TimeSeriesProperty specializations
  auto lhs_tsp_float = dynamic_cast<const TimeSeriesProperty<float> *>(&lhs);
  if (lhs_tsp_float)
    return lhs_tsp_float->operator==(rhs);

  auto lhs_tsp_double = dynamic_cast<const TimeSeriesProperty<double> *>(&lhs);
  if (lhs_tsp_double)
    return lhs_tsp_double->operator==(rhs);

  auto lhs_tsp_string = dynamic_cast<const TimeSeriesProperty<std::string> *>(&lhs);
  if (lhs_tsp_string)
    return lhs_tsp_string->operator==(rhs);

  auto lhs_tsp_bool = dynamic_cast<const TimeSeriesProperty<bool> *>(&lhs);
  if (lhs_tsp_bool)
    return lhs_tsp_bool->operator==(rhs);

  // use fallthrough behavior
  return (lhs.value() == rhs.value());
}

/**
 * @param lhs Thing on the left
 * @param rhs Thing on the right
 * @return true if they are not equal
 */
bool operator!=(const Property &lhs, const Property &rhs) { return (!(lhs == rhs)); }

/**
 * Get the unmangled name of the given typestring for some common types that we
 * use. Note that
 * this is just a lookup and NOT an unmangling algorithm
 * @param type :: A pointer to the type_info object for this type
 * @returns An unmangled version of the name
 */
std::string getUnmangledTypeName(const std::type_info &type) {
  using std::make_pair;
  using std::string;
  using namespace Mantid::API;
  using namespace Mantid::DataObjects;
  // Compile a lookup table. This is a static local variable that
  // will get initialized when the function is first used
  static std::unordered_map<string, string> typestrings;
  if (typestrings.empty()) {
    typestrings.emplace(typeid(char).name(), string("letter"));
    typestrings.emplace(typeid(int).name(), string("number"));
    typestrings.emplace(typeid(long long).name(), string("number"));
    typestrings.emplace(typeid(int64_t).name(), string("number"));
    typestrings.emplace(typeid(double).name(), string("number"));
    typestrings.emplace(typeid(bool).name(), string("boolean"));
    typestrings.emplace(typeid(string).name(), string("string"));
    typestrings.emplace(typeid(std::vector<string>).name(), string("str list"));
    typestrings.emplace(typeid(std::vector<int>).name(), string("int list"));
    typestrings.emplace(typeid(std::vector<long>).name(), string("long list"));
    typestrings.emplace(typeid(std::vector<int64_t>).name(), string("int list"));
    typestrings.emplace(typeid(std::vector<size_t>).name(), string("unsigned int list"));
    typestrings.emplace(typeid(std::vector<double>).name(), string("dbl list"));
    typestrings.emplace(typeid(std::vector<std::vector<string>>).name(), string("list of str lists"));
    typestrings.emplace(typeid(OptionalBool).name(), string("optional boolean"));

    // Workspaces
    typestrings.emplace(typeid(std::shared_ptr<Workspace>).name(), string("Workspace"));
    typestrings.emplace(typeid(std::shared_ptr<MatrixWorkspace>).name(), string("MatrixWorkspace"));
    typestrings.emplace(typeid(std::shared_ptr<ITableWorkspace>).name(), string("TableWorkspace"));
    typestrings.emplace(typeid(std::shared_ptr<IMDWorkspace>).name(), string("IMDWorkspace"));
    typestrings.emplace(typeid(std::shared_ptr<IMDEventWorkspace>).name(), string("MDEventWorkspace"));
    typestrings.emplace(typeid(std::shared_ptr<IEventWorkspace>).name(), string("IEventWorkspace"));
    typestrings.emplace(typeid(std::shared_ptr<Workspace2D>).name(), string("Workspace2D"));
    typestrings.emplace(typeid(std::shared_ptr<EventWorkspace>).name(), string("EventWorkspace"));
    typestrings.emplace(typeid(std::shared_ptr<PeaksWorkspace>).name(), string("PeaksWorkspace"));
    typestrings.emplace(typeid(std::shared_ptr<LeanElasticPeaksWorkspace>).name(), string("LeanElasticPeaksWorkspace"));
    typestrings.emplace(typeid(std::shared_ptr<IPeaksWorkspace>).name(), string("IPeaksWorkspace"));
    typestrings.emplace(typeid(std::shared_ptr<GroupingWorkspace>).name(), string("GroupingWorkspace"));
    typestrings.emplace(typeid(std::shared_ptr<WorkspaceGroup>).name(), string("WorkspaceGroup"));
    typestrings.emplace(typeid(std::shared_ptr<OffsetsWorkspace>).name(), string("OffsetsWorkspace"));
    typestrings.emplace(typeid(std::shared_ptr<MaskWorkspace>).name(), string("MaskWorkspace"));
    typestrings.emplace(typeid(std::shared_ptr<SpecialWorkspace2D>).name(), string("SpecialWorkspace2D"));
    typestrings.emplace(typeid(std::shared_ptr<IMDHistoWorkspace>).name(), string("IMDHistoWorkspace"));
    typestrings.emplace(typeid(std::shared_ptr<SplittersWorkspace>).name(), string("SplittersWorkspace"));
    typestrings.emplace(typeid(std::shared_ptr<SpecialWorkspace2D>).name(), string("SpecialWorkspace2D"));
    typestrings.emplace(typeid(std::shared_ptr<TableWorkspace>).name(), string("TableWorkspace"));
    // FunctionProperty
    typestrings.emplace(typeid(std::shared_ptr<IFunction>).name(), string("Function"));
    typestrings.emplace(typeid(std::shared_ptr<IAlgorithm>).name(), string("IAlgorithm"));
    typestrings.emplace(typeid(std::shared_ptr<PropertyManager>).name(), string("Dictionary"));
  }
  auto mitr = typestrings.find(type.name());
  if (mitr != typestrings.end()) {
    return mitr->second;
  }
  /* if a type name looks like
  N6Mantid6Kernel16EnumeratedStringINS_12_GLOBAL__N_111BinningModeEXadL_ZNS2_L16binningModeNamesEEEXadL_ZNS0_12_GLOBAL__N_114compareStringsEEEEE
  we assume it is a EnumeratedStringProperty and return a "string" for it */
  string type_name = type.name();
  if (type_name.find("Mantid") != std::string::npos && type_name.find("EnumeratedString") != std::string::npos) {
    return "string";
  }
  return type.name();
}

/**
 * Returns if the property is set to  automatically trim string unput values of
 * whitespace
 * @returns True/False
 */
bool Property::autoTrim() const { return m_autotrim; }

/**
 * Sets if the property is set to  automatically trim string unput values of
 * whitespace
 * @param setting The new setting value
 */
void Property::setAutoTrim(const bool &setting) { m_autotrim = setting; }

/**
 * Returns if the property is set to disable the creation of the "Replace Workspace" button
 * @returns True/False
 */
bool Property::disableReplaceWSButton() const { return m_disableReplaceWSButton; }

/**
 * Sets the property to disable the creation of the "Replace Workspace" button
 * @param disable The option to disable or not
 */
void Property::setDisableReplaceWSButton(const bool &disable) { m_disableReplaceWSButton = disable; }
} // namespace Kernel

} // namespace Mantid
