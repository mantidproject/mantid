#include "MantidAlgorithms/SubmitRemoteJob.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/MaskedProperty.h"

#include "MantidRemote/RemoteTask.h"
#include "MantidRemote/RemoteJobManager.h"

#include <sstream>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SubmitRemoteJob)

//using namespace Mantid::Kernel;
//using namespace Mantid::API;
//using namespace Mantid::Geometry;

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages

void SubmitRemoteJob::init()
{
  // Unlike most algorithms, this wone doesn't deal with workspaces....

  auto mustBePositive = boost::make_shared<Mantid::Kernel::BoundedValidator<int> >();
  mustBePositive->setLower(0);

  auto requireValue = boost::make_shared<Mantid::Kernel::MandatoryValidator<std::string> >();

  // TODO: need a better validator for the compute facility property
  declareProperty( "ComputeResource", "", requireValue, "", Mantid::Kernel::Direction::Input);
  declareProperty( "NumNodes", 0,  mustBePositive, "", Mantid::Kernel::Direction::Input);
  declareProperty( "CoresPerNode", 0,  mustBePositive, "", Mantid::Kernel::Direction::Input);
  // Number of actual MPI processes will be (NumNodes * CoresPerNode)

  // This is just an easy way to reference remote jobs (such as when we display a list of
  // all the jobs the user has submitted recently...)
  declareProperty( "TaskName", "", Mantid::Kernel::Direction::Input);

  // TODO: Can we figure out the user name/group name automatically?
  // (Group can probably be in the facilities.xml file.  Username can from
  // from the user's prefs file.  Password should never be written to disk ever!
  // Note that these are really implementation details: some other cluster
  // might want globus certificates or something like that...
  declareProperty( "UserName", "", requireValue, "", Mantid::Kernel::Direction::Input);
  declareProperty( "GroupName", "", requireValue, "", Mantid::Kernel::Direction::Input);

  // Password doesn't get echoed to the screen...
  declareProperty( new Mantid::Kernel::MaskedProperty<std::string>( "Password", "", requireValue, Mantid::Kernel::Direction::Input), "");

  // The transaction ID comes from the OpenTransaction algortithm (as yet unwritten...)
  declareProperty( "TransactionID", "", requireValue, "", Mantid::Kernel::Direction::Input);

  // Assuming the submission succeeded, this property will be set with a value
  // we can use to track the job
  declareProperty( "JobID", "", Mantid::Kernel::Direction::Output);

  // TODO: not sure we need these - we throw an exception when the submission fails
  // If there was a problem submitting the job, details will be stored in these properties
  declareProperty( "ErrorCode", "", Mantid::Kernel::Direction::Output);
  declareProperty( "ErrorMessage", "", Mantid::Kernel::Direction::Output);
}

void SubmitRemoteJob::exec()
{
  // Put the algorithm execution code here... 

  // The first thing to do will almost certainly be to retrieve the input workspace.
  // Here's the line for that - just uncomment it:
  //   MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");

  boost::shared_ptr<RemoteJobManager> jobManager = Mantid::Kernel::ConfigService::Instance().getFacility().getRemoteJobManager( getPropertyValue("ComputeResource"));

  // jobManager is a boost::shared_ptr...
  if (! jobManager)
  {
    // Requested compute resource doesn't exist
    g_log.error() << "Unable to create a compute resource named " <<  getPropertyValue("ComputeResource") << std::endl;
    g_log.error() << "Job not submitted" << std::endl;

    // TODO: should we create our own exception class for this??
    throw( new std::runtime_error( std::string("Unable to create a compute resource named " + getPropertyValue("ComputeResource"))));
  }


  // Create a RemoteTask object for this job
  RemoteTask task(getPropertyValue( "TaskName"), getPropertyValue( "TransactionID"));
  task.appendResource( "group", getPropertyValue( "GroupName"));
  task.appendResource( "nodes", getPropertyValue( "NumNodes"));

  // Need to figure out the total number of MPI processes
  unsigned coresPerNode, numNodes;
  std::stringstream convert( getPropertyValue( "NumNodes"));
  convert >> numNodes;
  convert.str( getPropertyValue( "CoresPerNode"));
  convert >> coresPerNode;

  convert.str("");
  convert << (numNodes * coresPerNode);

  // Set the username and password from the properties
  jobManager->setUserName( getPropertyValue ("UserName"));
  jobManager->setPassword( getPropertyValue( "Password"));

  // append command line options in the order that we want them displayed
  task.appendCmdLineParam( "-n");
  task.appendCmdLineParam(  convert.str());

  task.appendCmdLineParam( "-hostfile");
  task.appendCmdLineParam( "$PBS_NODEFILE");  // This is obviously specific to PBS...

  task.appendCmdLineParam( "/usr/bin/python");  // TODO: the python executable is stored in facilities.xml, but actually making
                                                // use of it would be very ugly (it's copied into MWSRemoteJobManager)

  // HACK! this is where we'd normally specify a file name for the python script to run...
  task.appendCmdLineParam( "-c");
  task.appendCmdLineParam( "print 'Hello World'");

  std::string retMsg;
  bool jobOutput = jobManager->submitJob( task, retMsg);

  if (jobOutput)
  {
    setPropertyValue( "JobID", retMsg);
    g_log.information() << "Job submitted.  JobID: " << retMsg << std::endl;
  }
  else
  {
    throw( new std::runtime_error( "Job submission failed: " + retMsg));
  }
}

