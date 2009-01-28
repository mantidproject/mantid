//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/XMLlogfile.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "muparser/muParser.h"
#include "MantidGeometry/Component.h"

#include <ctime>

namespace Mantid
{
namespace DataHandling
{

using namespace Kernel;
using namespace API;

Logger& XMLlogfile::g_log = Logger::get("XMLlogfile");

/// Constructor
XMLlogfile::XMLlogfile(std::string& logfileID, std::string& paramName, std::string& type, 
                       std::string& extractSingleValueAs, std::string& eq, Geometry::Component* comp) 
                       : m_logfileID(logfileID), m_paramName(paramName), m_type(type), 
                       m_extractSingleValueAs(extractSingleValueAs), m_eq(eq), m_component(comp)
{}


/** Returns parameter value as generated using possibly equation expression etc
 *
 *  @param logData Data in logfile
 *  @return parameter value
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML instrument definition file
 */
double XMLlogfile::createParamValue(TimeSeriesProperty<double>* logData) 
{  
  // Get for now just 1st entry of timeserie

  std::map<std::time_t, double> logMap = logData->valueAsMap();
  std::map<std::time_t, double> :: iterator it;
  it = logMap.begin(); 
  double firstEntry = (*it).second;


  // Check if m_eq is specified if yes evaluate this equation

  if ( m_eq.empty() )
    return firstEntry;
  else
  {
    size_t found;
    std::string equationStr = m_eq;
    found = equationStr.find("value");
    if ( found==std::string::npos )
    {
      throw Kernel::Exception::InstrumentDefinitionError(std::string("Equation attribute for <parameter>") 
        + " element (eq=" + m_eq + ") in instrument definition file must contain the string: \"value\"." 
        + ". \"value\" is replaced by a value from the logfile.");
    }

    std::stringstream readDouble;
    readDouble << firstEntry;
    std::string firstEntryStr = readDouble.str();
    equationStr.replace(found, 5, firstEntryStr);

    // check if more than one 'value' in m_eq

    while ( equationStr.find("value") != std::string::npos )
    {
      found = equationStr.find("value");
      equationStr.replace(found, 5, firstEntryStr);
    }

    try 
    {
      mu::Parser p;
      p.SetExpr(equationStr);
      return p.Eval();
    }
    catch (mu::Parser::exception_type &e)
    {
      throw Kernel::Exception::InstrumentDefinitionError(std::string("Equation attribute for <parameter>") 
        + " element (eq=" + m_eq + ") in instrument definition file cannot be parsed." 
        + ". Muparser error message is: " + e.GetMsg());
    }
  }


}



} // namespace DataHandling
} // namespace Mantid
