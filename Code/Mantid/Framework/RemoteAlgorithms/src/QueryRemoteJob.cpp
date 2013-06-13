#include "MantidRemoteAlgorithms/QueryRemoteJob.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/NullValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MaskedProperty.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"

#include "MantidRemote/RemoteJobManager.h"

#include "boost/make_shared.hpp"

namespace Mantid
{
namespace RemoteAlgorithms
{
    
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(QueryRemoteJob)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages

void QueryRemoteJob::init()
{
  // Unlike most algorithms, this one doesn't deal with workspaces....

  auto requireValue = boost::make_shared<MandatoryValidator<std::string> >();
  auto nullValidator = boost::make_shared<NullValidator>();
  auto jobStatusValidator = boost::make_shared<BoundedValidator <unsigned> >();
  jobStatusValidator->setLower( 0);
  jobStatusValidator->setUpper( (unsigned)RemoteJob::JOB_STATUS_UNKNOWN);

  // Compute Resources
  std::vector<std::string> computes = Mantid::Kernel::ConfigService::Instance().getFacility().computeResources();
  declareProperty( "ComputeResource", "", boost::make_shared<StringListValidator>(computes), "", Direction::Input);

  // The ID of the job we want to query
  declareProperty( "JobID", "", requireValue, "", Direction::Input);

  // TODO: Can we figure out the user name name automatically?
  declareProperty( "UserName", "", requireValue, "", Direction::Input);

  // Password doesn't get echoed to the screen...
  declareProperty( new MaskedProperty<std::string>( "Password", "", requireValue, Direction::Input), "");


  // A numeric code for the job's status
  declareProperty( "JobStatusCode", (unsigned)RemoteJob::JOB_STATUS_UNKNOWN,  jobStatusValidator, "", Direction::Output);
  
  // A human readable description of the job's status
  declareProperty( "JobStatusString", "", nullValidator, "",  Direction::Output);

}

void QueryRemoteJob::exec()
{
  boost::shared_ptr<RemoteJobManager> jobManager = Mantid::Kernel::ConfigService::Instance().getFacility().getRemoteJobManager( getPropertyValue("ComputeResource"));

  // jobManager is a boost::shared_ptr...
  if (! jobManager)
  {
    // Requested compute resource doesn't exist
    // TODO: should we create our own exception class for this??
    throw( std::runtime_error( std::string("Unable to create a compute resource named " + getPropertyValue("ComputeResource"))));
  }

  // Set the username and password from the properties
  jobManager->setUserName( getPropertyValue ("UserName"));
  jobManager->setPassword( getPropertyValue( "Password"));

  std::string errMsg;
  RemoteJob::JobStatus status;
  if (jobManager->jobStatus( getPropertyValue("JobID"), status, errMsg))
  {
    setProperty( "JobStatusCode", (unsigned)status);
    setProperty( "JobStatusString", mapStatusToString( status));
  }
  else
  {
    throw( std::runtime_error( "Error querying remote jobs: " + errMsg));
  }


  
}

std::string QueryRemoteJob::mapStatusToString( unsigned status)
{
  std::string retVal;
  switch (status)
  {
  case (unsigned)RemoteJob::JOB_COMPLETE:
    retVal = "Job complete";
    break;

  case (unsigned)RemoteJob::JOB_RUNNING:
    retVal = "Job running";
    break;

  case (unsigned)RemoteJob::JOB_QUEUED:
    retVal = "Job queued";
    break;

  case (unsigned)RemoteJob::JOB_ABORTED:
    retVal = "Job aborted";
    break;

  case (unsigned)RemoteJob::JOB_REMOVED:
    retVal = "Job removed";
    break;

  case (unsigned)RemoteJob::JOB_DEFERRED:
    retVal = "Job defferred";
    break;

  case (unsigned)RemoteJob::JOB_IDLE:
    retVal = "Job idle";
    break;

  case (unsigned)RemoteJob::JOB_STATUS_UNKNOWN:
    retVal = "Unknown job status";
    break;

  default:
    retVal = "Error querying job status.";
  }

  return retVal;
}

} // end namespace RemoteAlgorithms
} // end namespace Mantid
