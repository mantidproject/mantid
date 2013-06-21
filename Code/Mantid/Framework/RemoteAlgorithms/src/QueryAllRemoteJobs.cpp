#include "MantidRemoteAlgorithms/QueryAllRemoteJobs.h"
#include "MantidKernel/MandatoryValidator.h"
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
DECLARE_ALGORITHM(QueryAllRemoteJobs)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages

void QueryAllRemoteJobs::init()
{
  // Unlike most algorithms, this one doesn't deal with workspaces....

  auto requireValue = boost::make_shared<MandatoryValidator<std::string> >();
  auto nullValidator = boost::make_shared<NullValidator>();

  // Compute Resources
  std::vector<std::string> computes = Mantid::Kernel::ConfigService::Instance().getFacility().computeResources();
  declareProperty( "ComputeResource", "", boost::make_shared<StringListValidator>(computes), "", Direction::Input);

  // TODO: Can we figure out the user name name automatically?
  declareProperty( "UserName", "", requireValue, "", Direction::Input);

  // Password doesn't get echoed to the screen...
  declareProperty( new MaskedProperty<std::string>( "Password", "", requireValue, Direction::Input), "");

  // Mantid can't store arbitrary structs in its properties, so we're going to declare several
  // array properties for different pieces of data.  Values from the same array index are for
  // the same job.
  declareProperty( new ArrayProperty<std::string>("JobId", nullValidator, Direction::Output));
  declareProperty( new ArrayProperty<std::string>("JobStatusString", nullValidator, Direction::Output));
  declareProperty( new ArrayProperty<int>("JobStatusNum", nullValidator, Direction::Output));
  declareProperty( new ArrayProperty<std::string>("JobName", nullValidator, Direction::Output));
  declareProperty( new ArrayProperty<std::string>("JobStartTime", nullValidator, Direction::Output));
  declareProperty( new ArrayProperty<std::string>("JobCompletionTime", nullValidator, Direction::Output));
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

  // Set the username and password from the properties
  jobManager->setUserName( getPropertyValue ("UserName"));
  jobManager->setPassword( getPropertyValue( "Password"));

  std::string errMsg;
  std::vector<RemoteJob> jobList;

  if (jobManager->jobStatusAll( jobList, errMsg))
  {
    std::vector<std::string> jobIds;
    std::vector<std::string> jobStatusStrs;
    std::vector<int> jobStatusNums;
    std::vector<std::string> jobNames;
    std::vector<std::string> jobStartTimes;
    std::vector<std::string> jobCompletionTimes;
    for (unsigned i = 0; i < jobList.size(); i++)
    {
      jobIds.push_back(jobList[i].m_jobId);
      jobStatusStrs.push_back( jobList[i].statusString());
      jobStatusNums.push_back((int)jobList[i].m_status);
      jobNames.push_back(jobList[i].m_algName);
      jobStartTimes.push_back(jobList[i].m_startTime.toISO8601String());
      jobCompletionTimes.push_back(jobList[i].m_completionTime.toISO8601String());
    }

    setProperty( "JobId", jobIds);
    setProperty( "JobStatusString", jobStatusStrs);
    setProperty( "JobStatusNum", jobStatusNums);
    setProperty( "JobName", jobNames);
    setProperty( "JobStartTime", jobStartTimes);
    setProperty( "JobCompletionTime", jobCompletionTimes);
  }
  else
  {
    throw( std::runtime_error( "Error querying remote jobs: " + errMsg));
  }
  
}


} // end namespace RemoteAlgorithms
} // end namespace Mantid
