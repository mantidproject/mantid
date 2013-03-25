#include "MantidAlgorithms/SubmitRemoteJob.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/Exception.h"

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
  // Put your initialisation code (e.g. declaring properties) here...

  // Virtually all algorithms will want an input and an output workspace as properties.
  // Here are the lines for this, so just uncomment them:
  //   declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input));
  //   declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

  // Unlike most algorithms, we don't deal with workspaces.

  auto mustBePositive = boost::make_shared<Mantid::Kernel::BoundedValidator<int> >();
  mustBePositive->setLower(0);

  // TODO: need a validator for the compute facility property
  declareProperty( "ComputeResource", "", Mantid::Kernel::Direction::Input);
  declareProperty( "NumNodes", 0,  mustBePositive, "", Mantid::Kernel::Direction::Input);
  declareProperty( "CoresPerNode", 0,  mustBePositive, "", Mantid::Kernel::Direction::Input);
  // Number of actual MPI processes will be (NumNodes * CoresPerNode)

  // This is just an easy way to reference remote jobs (such as when we display a list of
  // all the jobs the user has submitted recently...)
  declareProperty( "TaskName", "", Mantid::Kernel::Direction::Input);

  // TODO: can we get the default location for mpirun from facilities.xml?
  declareProperty( "Executable", "/usr/bin/mpirun", Mantid::Kernel::Direction::Input);
  // we'll build the rest of the command line from other properties down in exec()

  // TODO: Can we figure out the user name/group name automatically?
  // (Group can probably be in the facilities.xml file.  Username can from
  // from the user's prefs file.  Password should never be written to disk ever!
  // Note that these are really implementation details: some other cluster
  // might want globus certificates or something like that...
  // TODO: Need a validator that doesn't allow empty strings...
  declareProperty( "UserName", "", Mantid::Kernel::Direction::Input);
  declareProperty( "GroupName", "", Mantid::Kernel::Direction::Input);
  declareProperty( "Password", "", Mantid::Kernel::Direction::Input);

  // The transaction ID comes from the OpenTransaction algortithm (as yet unwritten...)
  declareProperty( "TransactionID", "", Mantid::Kernel::Direction::Input);

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
  RemoteTask task(getPropertyValue( "TaskName"), getPropertyValue( "Executable"), getPropertyValue( "TransactionID"));
  task.appendResource( "Group", getPropertyValue( "GroupName"));
  task.appendResource( "Nodes", getPropertyValue( "NumNodes"));

  // Need to figure out the total number of MPI processes
  unsigned coresPerNode, numNodes;
  std::stringstream convert( getPropertyValue( "NumNodes"));
  convert >> numNodes;
  convert.str( getPropertyValue( "CoresPerNode"));
  convert >> coresPerNode;

  convert.str("");
  convert << (numNodes * coresPerNode);

  // append command line options in the order that we want them displayed
  task.appendCmdLineParam( "-n");
  task.appendCmdLineParam(  convert.str());

  task.appendCmdLineParam( "-hostfile");
  task.appendCmdLineParam( "$PBS_NODEFILE");  // This is obviously specific to PBS...

  task.appendCmdLineParam( "/usr/bin/python");  // TODO:  store the location of the python interpreter in facilities.xml??

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

