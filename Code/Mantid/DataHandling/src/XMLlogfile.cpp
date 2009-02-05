//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/XMLlogfile.h"
#include "MantidGeometry/Component.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <muparser/muParser.h>
#include <ctime>
#include "MantidDataHandling/LogParser.h"

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

  //std::map<std::time_t, double> logMap = logData->valueAsMap();
  //std::map<std::time_t, double> :: iterator it;
  //it = logMap.begin(); 
  double extractedValue; // = nthValue(logData, 1); //(*it).second;


  // get value from time series

  if ( m_extractSingleValueAs.compare("mean" ) == 0 )
  {
    extractedValue = timeMean(logData);
  }
  // Looking for string: "position n", where n is an integer
  else if ( m_extractSingleValueAs.find("position") == 0 && m_extractSingleValueAs.size() >= 10 )  
  {
    std::stringstream extractPosition(m_extractSingleValueAs);  
    std::string dummy;
    int position;
    extractPosition >> dummy >> position;

    extractedValue = nthValue(logData, position);
  }
  else
  {
    throw Kernel::Exception::InstrumentDefinitionError(std::string("extract-single-value-as attribute for <parameter>") 
        + " element (eq=" + m_eq + ") in instrument definition file is not recognised.");
  }

  // Check if m_eq is specified if yes evaluate this equation

  if ( m_eq.empty() )
    return extractedValue;
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
    readDouble << extractedValue;
    std::string extractedValueStr = readDouble.str();
    equationStr.replace(found, 5, extractedValueStr);

    // check if more than one 'value' in m_eq

    while ( equationStr.find("value") != std::string::npos )
    {
      found = equationStr.find("value");
      equationStr.replace(found, 5, extractedValueStr);
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
