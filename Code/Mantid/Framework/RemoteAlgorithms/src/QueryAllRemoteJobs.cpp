#include "MantidRemoteAlgorithms/QueryAllRemoteJobs.h"
#include "MantidKernel/NullValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"

#include "MantidKernel/RemoteJobManager.h"
#include "MantidRemote/SimpleJSON.h"

#include "boost/make_shared.hpp"

namespace Mantid
{
namespace RemoteAlgorithms
{
    
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(QueryAllRemoteJobs)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages

void QueryAllRemoteJobs::init()
{
  // Unlike most algorithms, this one doesn't deal with workspaces....

  auto nullValidator = boost::make_shared<NullValidator>();

  // Compute Resources
  std::vector<std::string> computes = Mantid::Kernel::ConfigService::Instance().getFacility().computeResources();
  declareProperty( "ComputeResource", "", boost::make_shared<StringListValidator>(computes), "", Direction::Input);

  // Mantid can't store arbitrary structs in its properties, so we're going to declare several
  // array properties for different pieces of data.  Values from the same array index are for
  // the same job.
  declareProperty( new ArrayProperty<std::string>("JobId", nullValidator, Direction::Output));
  declareProperty( new ArrayProperty<std::string>("JobStatusString", nullValidator, Direction::Output));
  declareProperty( new ArrayProperty<std::string>("JobName", nullValidator, Direction::Output));
  declareProperty( new ArrayProperty<std::string>("ScriptName", nullValidator, Direction::Output));
  declareProperty( new ArrayProperty<std::string>("TransID", nullValidator, Direction::Output));

// These aren't implemented on the server side, yet
//  declareProperty( new ArrayProperty<std::string>("JobStartTime", nullValidator, Direction::Output));
//  declareProperty( new ArrayProperty<std::string>("JobCompletionTime", nullValidator, Direction::Output));
}

void QueryAllRemoteJobs::exec()
{
  boost::shared_ptr<RemoteJobManager> jobManager = Mantid::Kernel::ConfigService::Instance().getFacility().getRemoteJobManager( getPropertyValue("ComputeResource"));

  // jobManager is a boost::shared_ptr...
  if (! jobManager)
  {
    // Requested compute resource doesn't exist
    // TODO: should we create our own exception class for this??
    throw( std::runtime_error( std::string("Unable to create a compute resource named " + getPropertyValue("ComputeResource"))));
  }

  std::istream &respStream = jobManager->httpGet("/query");
  JSONObject resp;
  initFromStream( resp, respStream);
  if (jobManager->lastStatus() == Poco::Net::HTTPResponse::HTTP_OK)
  {
    std::vector<std::string> jobIds;
    std::vector<std::string> jobStatusStrs;
    std::vector<std::string> jobNames;
    std::vector<std::string> scriptNames;
    std::vector<std::string> transIds;

// These haven't been implemented on the server side yet
//    std::vector<std::string> jobStartTimes;
//    std::vector<std::string> jobCompletionTimes;

    JSONObject::const_iterator it = resp.begin();
    while (it != resp.end())
    {
      jobIds.push_back( (*it).first);
      JSONObject jobData;
      (*it).second.getValue(jobData);

      std::string value;
      jobData["JobStatus"].getValue( value);
      jobStatusStrs.push_back( value);

      jobData["JobName"].getValue( value);
      jobNames.push_back(value);

      jobData["ScriptName"].getValue( value);
      scriptNames.push_back(value);

      jobData["TransID"].getValue( value);
      transIds.push_back(value);

      it++;
    }

    setProperty( "JobId", jobIds);
    setProperty( "JobStatusString", jobStatusStrs);
    setProperty( "JobName", jobNames);
    setProperty( "ScriptName", scriptNames);
    setProperty( "TransID", transIds);

//    setProperty( "JobStartTime", jobStartTimes);
//    setProperty( "JobCompletionTime", jobCompletionTimes);

  }
  else
  {
    std::string errMsg;
    resp["Err_Msg"].getValue( errMsg);
    throw( std::runtime_error( errMsg));
  }
  
}


} // end namespace RemoteAlgorithms
} // end namespace Mantid
