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
  try 
  {
    mu::Parser p;

    double x = 1;

    p.DefineVar(std::string("x"), &x);
    //parser.DefineVar("my_var", var);
    p.SetExpr("1+1");
   //std::cout << p.Eval() << std::endl;
  }
  catch (mu::Parser::exception_type &e)
  {
    throw Kernel::Exception::InstrumentDefinitionError(std::string("Equation attribute for <parameter>") 
      + " element (eq=" + m_eq + ") in instrument definition file cannot be parsed." 
      + ". Muparser error message is: " + e.GetMsg());
  }


  std::map<std::time_t, double> logMap = logData->valueAsMap();

  std::map<std::time_t, double> :: iterator it;

  it = logMap.begin(); // for now just return 1st entry

  return (*it).second;
}



} // namespace DataHandling
} // namespace Mantid
