/*WIKI*

Submit a job to be executed on the specified remote compute resource.

*WIKI*/

#include "MantidRemoteAlgorithms/SubmitRemoteJob.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/MaskedProperty.h"
#include "MantidKernel/ListValidator.h"

#include "MantidRemote/RemoteTask.h"
#include "MantidRemote/RemoteJobManager.h"

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

namespace Mantid
{
namespace RemoteAlgorithms
{


// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SubmitRemoteJob)

using namespace Mantid::Kernel;
//using namespace Mantid::API;
//using namespace Mantid::Geometry;

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages

void SubmitRemoteJob::init()
{
  // Unlike most algorithms, this wone doesn't deal with workspaces....

  auto mustBePositive = boost::make_shared<BoundedValidator<int> >();
  mustBePositive->setLower(0);

  auto requireValue = boost::make_shared<MandatoryValidator<std::string> >();

  // Compute Resources
  std::vector<std::string> computes = Mantid::Kernel::ConfigService::Instance().getFacility().computeResources();
  declareProperty( "ComputeResource", "", boost::make_shared<StringListValidator>(computes), "", Direction::Input);

  declareProperty( "NumNodes", 0,  mustBePositive, "", Direction::Input);
  declareProperty( "CoresPerNode", 0,  mustBePositive, "", Direction::Input);
  // Number of actual MPI processes will be (NumNodes * CoresPerNode)

  // This is just an easy way to reference remote jobs (such as when we display a list of
  // all the jobs the user has submitted recently...)
  declareProperty( "TaskName", "", Direction::Input);

  // TODO: Can we figure out the user name/group name automatically?
  // (Group can probably be in the facilities.xml file.  Username can from
  // from the user's prefs file.  Password should never be written to disk ever!
  // Note that these are really implementation details: some other cluster
  // might want globus certificates or something like that...
  declareProperty( "UserName", "", requireValue, "", Direction::Input);
  declareProperty( "GroupName", "", requireValue, "", Direction::Input);

  // Password doesn't get echoed to the screen...
  declareProperty( new MaskedProperty<std::string>( "Password", "", requireValue, Direction::Input), "");

  // The transaction ID comes from the StartRemoteTransaction algortithm
  declareProperty( "TransactionID", "", requireValue, "", Direction::Input);

  // Assuming the submission succeeded, this property will be set with a value
  // we can use to track the job
  declareProperty( "JobID", "", Direction::Output);

  // Name of the python script to execute
  declareProperty( "ScriptName", "", requireValue, "", Direction::Input);

  // Command line arguments for the script
  declareProperty( "ScriptArguments", "", Direction::Input);
}

void SubmitRemoteJob::exec()
{
  // Put the algorithm execution code here... 

  // The first thing to do will almost certainly be to retrieve the input workspace.
  // Here's the line for that - just uncomment it:
  //   MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");

  boost::shared_ptr<RemoteJobManager> jobManager = ConfigService::Instance().getFacility().getRemoteJobManager( getPropertyValue("ComputeResource"));

  // jobManager is a boost::shared_ptr...
  if (! jobManager)
  {
    // Requested compute resource doesn't exist
    // TODO: should we create our own exception class for this??
    throw( std::runtime_error( std::string("Unable to create a compute resource named " + getPropertyValue("ComputeResource"))));
  }


  // Create a RemoteTask object for this job
  RemoteTask task(getPropertyValue( "TaskName"), getPropertyValue( "TransactionID"));
  task.appendResource( "group", getPropertyValue( "GroupName"));
  task.appendResource( "num_nodes", getPropertyValue( "NumNodes"));
  task.appendResource( "cores_per_node", getPropertyValue( "CoresPerNode"));
  task.appendResource ( "executable", getPropertyValue( "ScriptName"));

  // Set the username and password from the properties
  jobManager->setUserName( getPropertyValue ("UserName"));
  jobManager->setPassword( getPropertyValue( "Password"));

  // append command line options
  task.appendCmdLineParam( getPropertyValue( "ScriptArguments"));

  std::string retMsg;
  bool jobOutput = jobManager->submitJob( task, retMsg);

  if (jobOutput)
  {
    setPropertyValue( "JobID", retMsg);
    g_log.information() << "Job submitted.  JobID: " << retMsg << std::endl;
  }
  else
  {
    throw( std::runtime_error( "Job submission failed: " + retMsg));
  }
}

} // end namespace RemoteAlgorithms
} // end namespace Mantid