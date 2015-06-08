#include "MantidAPI/RemoteJobManagerFactory.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"
#include "MantidRemoteAlgorithms/SubmitRemoteJob2.h"

namespace Mantid {
namespace RemoteAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SubmitRemoteJob2)

using namespace Mantid::Kernel;

void SubmitRemoteJob2::init() {
  // Unlike most algorithms, this wone doesn't deal with workspaces....

  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(1);

  auto requireValue = boost::make_shared<MandatoryValidator<std::string>>();

  // Compute Resources
  std::vector<std::string> computes = Mantid::Kernel::ConfigService::Instance()
                                          .getFacility()
                                          .computeResources();
  declareProperty(
      "ComputeResource", "", boost::make_shared<StringListValidator>(computes),
      "The name of the remote computer to submit the job to", Direction::Input);

  // Note: these 2 properties are 'implementation specific'.  We know that for
  // example Fermi  needs them, and SCARF supports them, but we really
  // ought to query the information URL before requiring them.
  declareProperty("NumNodes", 1, mustBePositive,
                  "The number of compute nodes the job requires",
                  Direction::Input);
  declareProperty("CoresPerNode", 1, mustBePositive,
                  "The number of processes to start on each compute node",
                  Direction::Input);
  // Number of actual MPI processes will be (NumNodes * CoresPerNode)

  // This is just an easy way to reference remote jobs (such as when we display
  // a list of all the jobs the user has submitted recently...)
  declareProperty("TaskName", std::string(""),
                  "A short name for the job (optional).", Direction::Input);

  // The transaction ID comes from the StartRemoteTransaction algortithm
  declareProperty("TransactionID", "", requireValue,
                  "The transaction ID to associate with this job",
                  Direction::Input);

  // Name of the python script to execute
  declareProperty(
      "ScriptName", "", requireValue,
      "A name for the runnable/executable (for example a python script) "
      "that will be executed",
      Direction::Input);

  // The actual python code
  declareProperty(
      "ScriptParams", "", requireValue,
      "Parameters to pass to the runnable/script/executable - when running "
      "python scripts through the the Mantid remote job submission "
      "API this will be the actual python code to execute",
      Direction::Input);

  // Assuming the submission succeeded, this property will be set with a value
  // we can use to track the job
  declareProperty("JobID", std::string(""), "An ID string for this job",
                  Direction::Output);
}

void SubmitRemoteJob2::exec() {
  // Put the algorithm execution code here...
  const std::string comp = getPropertyValue("ComputeResource");
  Mantid::API::IRemoteJobManager_sptr jm =
      Mantid::API::RemoteJobManagerFactory::Instance().create(
          comp);

  const std::string tid = getPropertyValue("TransactionID");
  const std::string runnable = getPropertyValue("ScriptName");
  const std::string params = getPropertyValue("ScriptParams");
  const std::string displayName = getPropertyValue("TaskName");
  const int nodes = getProperty("NumNodes");
  const int cores = getProperty("CoresPerNode");
  std::string jid = jm->submitRemoteJob(tid, runnable, params, displayName, nodes, cores);

  setPropertyValue("JobID", jid);
  g_log.information() << "Job submitted.  Job ID =  "
                      << jid << " on (remote) compute resource " << comp << std::endl;
}

} // end namespace RemoteAlgorithms
} // end namespace Mantid
