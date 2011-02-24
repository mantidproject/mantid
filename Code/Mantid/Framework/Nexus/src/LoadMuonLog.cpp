//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/LoadMuonLog.h"
#include "MantidNexus/MuonNexusReader.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/LogParser.h"

#include <ctime>

namespace Mantid
{
namespace NeXus
{

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadMuonLog)

using namespace Kernel;
using Geometry::IInstrument;
using Geometry::IInstrument_sptr;
using API::WorkspaceProperty;
using API::MatrixWorkspace;
using API::MatrixWorkspace_sptr;
using API::FileProperty;
using DataObjects::Workspace2D;
using DataObjects::Workspace2D_sptr;
using API::Progress;

/// Empty default constructor
LoadMuonLog::LoadMuonLog()
{}

/// Initialisation method.
void LoadMuonLog::init()
{
  // When used as a sub-algorithm the workspace name is not used - hence the "Anonymous" to satisfy the validator
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("Workspace","Anonymous",Direction::InOut),
    "Name of the Muon Nexus file to read, as a full or relative path" );
  declareProperty(new FileProperty("Filename","", FileProperty::Load),
    "The name of the workspace to which the log data will be added" );
}

/** Executes the algorithm. Reading in Log entries from the Nexus file
 *
 *  @throw Mantid::Kernel::Exception::FileError  Thrown if file is not recognised to be a Nexus datafile
 *  @throw std::runtime_error Thrown with Workspace problems
 */
void LoadMuonLog::exec()
{
  // Retrieve the filename from the properties and perform some initial checks on the filename

  m_filename = getPropertyValue("Filename");

  MuonNexusReader nxload;
  if ( nxload.readLogData(m_filename) != 0 )
  {
    g_log.error("In LoadMuonLog: " + m_filename + " can not be opened.");
    throw Exception::FileError("File does not exist:" , m_filename);
  }

  // Get the input workspace and retrieve sample from workspace.
  // the log data will be loaded into the Sample container of the workspace
  // Also set the sample name at this point, as part of the sample related log data.

  const MatrixWorkspace_sptr localWorkspace = getProperty("Workspace");
  localWorkspace->mutableSample().setName(nxload.getSampleName());

  // Attempt to load the content of each NXlog section into the Sample object
  // Assumes that MuonNexusReader has read all log data
  // Two cases of double or string data allowed
 Progress prog(this,0.0,1.0,nxload.numberOfLogs());
  for (int i = 0; i < nxload.numberOfLogs(); i++)
  {
    std::string logName=nxload.getLogName(i);
    TimeSeriesProperty<double> *l_PropertyDouble = new TimeSeriesProperty<double>(logName);
    TimeSeriesProperty<std::string> *l_PropertyString = new TimeSeriesProperty<std::string>(logName);
    std::vector<double> logTimes;

    // Read log file into Property which is then stored in Sample object
    if(!nxload.logTypeNumeric(i))
    {
       std::string logValue;
       std::time_t logTime;
       for( int j=0;j<nxload.getLogLength(i);j++)
       {
          nxload.getLogStringValues(i,j,logTime,logValue);
          l_PropertyString->addValue(logTime, logValue);
       }
    }
    else
    {
       double logValue;
       std::time_t logTime;
       for( int j=0;j<nxload.getLogLength(i);j++)
       {
          nxload.getLogValues(i,j,logTime,logValue);
          l_PropertyDouble->addValue(logTime, logValue);
       }
    }

    // store Property in Sample object and delete unused object
    if ( nxload.logTypeNumeric(i) )
    {
      localWorkspace->mutableRun().addLogData(l_PropertyDouble);
      delete l_PropertyString;
    }
    else
    {
      localWorkspace->mutableRun().addLogData(l_PropertyString);
      delete l_PropertyDouble;
    }
    prog.report();
  } // end for


  // operation was a success and ended normally
  return;
}


/** change each element of the string to lower case
* @param strToConvert :: The input string
* @returns The string but with all characters in lower case
*/
std::string LoadMuonLog::stringToLower(std::string strToConvert)
{
  for(unsigned int i=0;i<strToConvert.length();i++)
  {
    strToConvert[i] = tolower(strToConvert[i]);
  }
  return strToConvert; //return the converted string
}


/** check if first 19 characters of a string is data-time string according to yyyy-mm-ddThh:mm:ss
* @param str :: The string to test
* @returns true if the strings format matched the expected date format
*/
bool LoadMuonLog::isDateTimeString(const std::string& str)
{
  if ( str.size() >= 19 )
  if ( str.compare(4,1,"-") == 0 && str.compare(7,1,"-") == 0 && str.compare(13,1,":") == 0
      && str.compare(16,1,":") == 0 && str.compare(10,1,"T") == 0 )
  return true;

  return false;
}

} // namespace NeXus
} // namespace Mantid
