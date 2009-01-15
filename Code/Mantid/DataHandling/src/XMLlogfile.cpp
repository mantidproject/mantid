//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/XMLlogfile.h"
#include "MantidKernel/TimeSeriesProperty.h"

//#include "MantidGeometry/CompAssembly.h"
#include "MantidGeometry/Component.h"
//#include "MantidKernel/PhysicalConstants.h"

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
 */
double XMLlogfile::createParamValue(TimeSeriesProperty<double>* logData) 
{

  std::map<std::time_t, double> logMap = logData->valueAsMap();

  std::map<std::time_t, double> :: iterator it;

  it = logMap.begin(); // for now just return 1st entry

  return (*it).second;
}



} // namespace DataHandling
} // namespace Mantid
