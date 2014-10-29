#include "MantidKernel/Property.h"

#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/IPropertySettings.h"
#include "MantidKernel/PropertyHistory.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <map>
#include <sstream>

namespace Mantid
{
namespace Kernel
{

/** Constructor
 *  @param name :: The name of the property
 *  @param type :: The type of the property
 *  @param direction :: Whether this is a Direction::Input, Direction::Output or Direction::InOut (Input & Output) property
 */
Property::Property( const std::string &name, const std::type_info &type, const unsigned int direction ) :
  m_name( name ),
  m_documentation( "" ),
  m_typeinfo( &type ),
  m_direction( direction ),
  m_units(""),
  m_settings(NULL),
  m_group(""),
  m_remember(true)
{
    // Make sure a random int hasn't been passed in for the direction
    // Property & PropertyWithValue destructors will be called in this case
    if (m_direction > 2) throw std::out_of_range("direction should be a member of the Direction enum");
}

/// Copy constructor
Property::Property( const Property& right ) :
  m_name( right.m_name ),
  m_documentation( right.m_documentation ),
  m_typeinfo( right.m_typeinfo ),
  m_direction( right.m_direction ),
  m_units( right.m_units),
  m_settings(NULL),
  m_group( right.m_group),
  m_remember(right.m_remember)
{
  if (right.m_settings)
    m_settings = right.m_settings->clone();
}

/// Virtual destructor
Property::~Property()
{
  if (m_settings)
    delete m_settings;
}

/** Get the property's name
 *  @return The name of the property
 */
const std::string& Property::name() const
{
  return m_name;
}

/** Get the property's documentation string
 *  @return The documentation string
 */
const std::string& Property::documentation() const
{
  return m_documentation;
}

/** Get the property's short documentation string
 *  @return The documentation string
 */
const std::string& Property::briefDocumentation() const
{
  return m_shortDoc;
}

/** Get the property type_info
 *  @return The type of the property
 */
const std::type_info* Property::type_info() const
{
  return m_typeinfo;
}

/** Returns the type of the property as a string.
 *  Note that this is implementation dependent.
 *  @return The property type
 */
const std::string Property::type() const
{
  return Mantid::Kernel::getUnmangledTypeName(*m_typeinfo);
}

/** Overridden functions checks whether the property has a valid value.
 *  
 *  @return empty string ""
 */
std::string Property::isValid() const
{
  // the no error condition
  return "";
}

/** 
 * Set the PropertySettings determining when this property is visible/enabled.
 * Takes ownership of the given object
 * @param settings A pointer to an object specifying the settings type 
 */
void Property::setSettings(IPropertySettings * settings)
{ 
  if(m_settings) delete m_settings;
  m_settings = settings;
}

/**
 *
 * @return the PropertySettings for this property
 */
IPropertySettings * Property::getSettings()
{
  return m_settings;
}

/**
 * Deletes the PropertySettings object contained
 */
void Property::deleteSettings()
{
  delete m_settings;
  m_settings = NULL;
}

/**
* Whether to remember this property input
* @return whether to remember this property's input
*/
bool Property::remember() const
{
  return m_remember;
}

/**
 * Set wheter to remeber this property input
 * @param remember :: true to remember
 */
void Property::setRemember(bool remember)
{
    m_remember=remember;
}

/** Sets the user level description of the property.
 *  In addition, if the brief documentation string is empty it will be set to
 *  the portion of the provided string up to the first period
 *  (or the entire string if no period is found).
 *  @param documentation The string containing the descriptive comment
 */
void Property::setDocumentation( const std::string& documentation )
{
  m_documentation = documentation;

  if ( m_shortDoc.empty() )
  {
    auto period = documentation.find_first_of('.');
    setBriefDocumentation( documentation.substr(0, period) );
  }
}

/** Sets the
 *
 */
void Property::setBriefDocumentation( const std::string& documentation )
{
  m_shortDoc = documentation;
}

/** Returns the set of valid values for this property, if such a set exists.
 *  If not, it returns an empty set.
 * @return the set of valid values for this property or an empty set
 */
std::vector<std::string> Property::allowedValues() const
{
  return std::vector<std::string>();
}

/// Create a PropertyHistory object representing the current state of the Property.
const PropertyHistory Property::createHistory() const
{
  return PropertyHistory(this);
}

/** Creates a temporary property value based on the memory address of 
 *  the property.
 */
void Property::createTemporaryValue()
{
  std::ostringstream os;
  os << "__TMP" << this;
  this->setValue(os.str());
}

/** Checks if the property value is a temporary one based on the memory address of 
 *  the property.
 */
bool Property::hasTemporaryValue() const
{
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
int Property::size() const
{
  return 1;
}

//-------------------------------------------------------------------------------------------------
/** Returns the units of the property, if any, as a string.
 * Units are optional, and will return empty string if they have
 * not been set before.
 * @return the property's units
 */
const std::string & Property::units() const
{
  return m_units;
}

//-------------------------------------------------------------------------------------------------
/** Sets the units of the property, as a string. This is optional.
 *
 * @param unit :: string to set for the units.
 */
void Property::setUnits(const std::string & unit)
{
  m_units = unit;
}

//-------------------------------------------------------------------------------------------------
/** Filter out a property by time. Will be overridden by TimeSeriesProperty (only)
 * @param start :: the beginning time to filter from
 * @param stop :: the ending time to filter to
 * */
void Property::filterByTime(const Kernel::DateAndTime &start, const Kernel::DateAndTime &stop)
{
  UNUSED_ARG(start);
  UNUSED_ARG(stop);
  //Do nothing in general
  return;
}


//-----------------------------------------------------------------------------------------------
/** Split a property by time. Will be overridden by TimeSeriesProperty (only)
 * For any other property type, this does nothing.
 * @param splitter :: time splitter
 * @param outputs :: holder for splitter output
 */
void Property::splitByTime(std::vector< SplittingInterval >& splitter, std::vector< Property * > outputs) const
{
  UNUSED_ARG(splitter);
  UNUSED_ARG(outputs);
  return;
}

} // End Kernel namespace


//-------------------------- Utility function for class name lookup -----------------------------

// MG 16/07/09: Some forward declarations.I need this so
// that the typeid function in getUnmangledTypeName knows about them
// This way I don't need to actually include the headers and I don't
// introduce unwanted dependencies
namespace API
{
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
}
namespace DataObjects
{
  class EventWorkspace;
  class PeaksWorkspace;
  class GroupingWorkspace;
  class OffsetsWorkspace;
  class MaskWorkspace;
  class SpecialWorkspace2D;
  class Workspace2D;
  class TableWorkspace;
  class SpecialWorkspace2D;
  class SplittersWorkspace;
}

namespace Kernel
{

/**
 * @param lhs Thing on the left
 * @param rhs Thing on the right
 * @return true if they are equal
 */
bool operator==( const Property & lhs, const Property & rhs )
{
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
bool operator!=( const Property & lhs, const Property & rhs )
{
  return (!(lhs == rhs));
}

/**
 * Get the unmangled name of the given typestring for some common types that we use. Note that
 * this is just a lookup and NOT an unmangling algorithm
 * @param type :: A pointer to the type_info object for this type
 * @returns An unmangled version of the name
 */
std::string getUnmangledTypeName(const std::type_info& type)
{
  using std::string;
  using std::make_pair;
  using namespace Mantid::API;
  using namespace Mantid::DataObjects;
  // Compile a lookup table. This is a static local variable that
  // will get initialized when the function is first used
  static std::map<string, string> typestrings;
  if( typestrings.empty() ) 
  {
    typestrings.insert(make_pair(typeid(char).name(), string("letter")));
    typestrings.insert(make_pair(typeid(int).name(), string("number")));
    typestrings.insert(make_pair(typeid(long long).name(), string("number")));
    typestrings.insert(make_pair(typeid(int64_t).name(), string("number")));
    typestrings.insert(make_pair(typeid(double).name(), string("number")));
    typestrings.insert(make_pair(typeid(bool).name(), string("boolean")));
    typestrings.insert(make_pair(typeid(string).name(), string("string")));
    typestrings.insert(make_pair(typeid(std::vector<string>).name(), string("str list")));
    typestrings.insert(make_pair(typeid(std::vector<int>).name(), string("int list")));
    typestrings.insert(make_pair(typeid(std::vector<int64_t>).name(), string("int list")));
    typestrings.insert(make_pair(typeid(std::vector<size_t>).name(), string("unsigned int list")));
    typestrings.insert(make_pair(typeid(std::vector<double>).name(), string("dbl list")));
    typestrings.insert(make_pair(typeid(std::vector<std::vector<string> >).name(), string("list of str lists")));

    //Workspaces
    typestrings.insert(make_pair(typeid(boost::shared_ptr<Workspace>).name(), 
                                      string("Workspace")));
    typestrings.insert(make_pair(typeid(boost::shared_ptr<MatrixWorkspace>).name(), 
                                      string("MatrixWorkspace")));
    typestrings.insert(make_pair(typeid(boost::shared_ptr<ITableWorkspace>).name(), 
                                      string("TableWorkspace")));
    typestrings.insert(make_pair(typeid(boost::shared_ptr<IMDWorkspace>).name(), 
                                      string("IMDWorkspace")));
    typestrings.insert(make_pair(typeid(boost::shared_ptr<IMDEventWorkspace>).name(), 
                                      string("MDEventWorkspace")));
    typestrings.insert(make_pair(typeid(boost::shared_ptr<IEventWorkspace>).name(), 
                                      string("IEventWorkspace")));
    typestrings.insert(make_pair(typeid(boost::shared_ptr<Workspace2D>).name(), 
                                      string("Workspace2D")));
    typestrings.insert(make_pair(typeid(boost::shared_ptr<EventWorkspace>).name(), 
                                      string("EventWorkspace")));
    typestrings.insert(make_pair(typeid(boost::shared_ptr<PeaksWorkspace>).name(), 
                                      string("PeaksWorkspace")));
    typestrings.insert(make_pair(typeid(boost::shared_ptr<IPeaksWorkspace>).name(),
                                      string("IPeaksWorkspace")));
    typestrings.insert(make_pair(typeid(boost::shared_ptr<GroupingWorkspace>).name(), 
                                      string("GroupingWorkspace")));
    typestrings.insert(make_pair(typeid(boost::shared_ptr<WorkspaceGroup>).name(),
                                      string("WorkspaceGroup")));
    typestrings.insert(make_pair(typeid(boost::shared_ptr<OffsetsWorkspace>).name(),
                                      string("OffsetsWorkspace")));
    typestrings.insert(make_pair(typeid(boost::shared_ptr<MaskWorkspace>).name(),
                                      string("MaskWorkspace")));
    typestrings.insert(make_pair(typeid(boost::shared_ptr<SpecialWorkspace2D>).name(), 
                                      string("SpecialWorkspace2D")));
    typestrings.insert(make_pair(typeid(boost::shared_ptr<IMDHistoWorkspace>).name(),
                                      string("IMDHistoWorkspace")));
    typestrings.insert(make_pair(typeid(boost::shared_ptr<SplittersWorkspace>).name(),
                                       string("SplittersWorkspace")));
    typestrings.insert(make_pair(typeid(boost::shared_ptr<SpecialWorkspace2D>).name(),
                                       string("SpecialWorkspace2D")));
    typestrings.insert(make_pair(typeid(boost::shared_ptr<TableWorkspace>).name(),
                                        string("TableWorkspace")));
    // FunctionProperty
    typestrings.insert(make_pair(typeid(boost::shared_ptr<IFunction>).name(),
                                        string("Function")));
    typestrings.insert(make_pair(typeid(boost::shared_ptr<IAlgorithm>).name(), string("IAlgorithm")));

  }
  std::map<std::string, std::string>::const_iterator mitr = typestrings.find(type.name());
  if( mitr != typestrings.end() )
  {
    return mitr->second;
  }

  return type.name();

}

} // namespace Kernel

} // namespace Mantid
