#include "MantidRemoteAlgorithms/SubmitRemoteJob.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ListValidator.h"

#include "MantidRemoteAlgorithms/SimpleJSON.h"
#include "MantidKernel/RemoteJobManager.h"

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

namespace Mantid {
namespace RemoteAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SubmitRemoteJob)

using namespace Mantid::Kernel;
// using namespace Mantid::API;
// using namespace Mantid::Geometry;

// A reference to the logger is provided by the base class, it is called g_log.

void SubmitRemoteJob::init() {
  // Unlike most algorithms, this wone doesn't deal with workspaces....

  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);

  auto requireValue = boost::make_shared<MandatoryValidator<std::string>>();

  // Compute Resources
  std::vector<std::string> computes = Mantid::Kernel::ConfigService::Instance()
                                          .getFacility()
                                          .computeResources();
  declareProperty(
      "ComputeResource", "", boost::make_shared<StringListValidator>(computes),
      "The name of the remote computer to submit the job to", Direction::Input);

  // Note: these 2 properties are 'implementation specific'.  We know that Fermi
  // needs them, but we really
  // ought to query the information URL before requiring them.
  declareProperty("NumNodes", 0, mustBePositive,
                  "The number of compute nodes the job requires",
                  Direction::Input);
  declareProperty("CoresPerNode", 0, mustBePositive,
                  "The number of processes to start on each compute node",
                  Direction::Input);
  // Number of actual MPI processes will be (NumNodes * CoresPerNode)

  // This is just an easy way to reference remote jobs (such as when we display
  // a list of
  // all the jobs the user has submitted recently...)
  declareProperty("TaskName", std::string(""), "A short name for the job.",
                  Direction::Input);

  // The transaction ID comes from the StartRemoteTransaction algortithm
  declareProperty("TransactionID", "", requireValue,
                  "The transaction ID to associate with this job",
                  Direction::Input);

  // Name of the python script to execute
  declareProperty("ScriptName", "", requireValue,
                  "A name for the python script that will be executed",
                  Direction::Input);

  // The actual python code
  declareProperty("PythonScript", "", requireValue,
                  "The actual python code to execute", Direction::Input);

  // Assuming the submission succeeded, this property will be set with a value
  // we can use to track the job
  declareProperty("JobID", std::string(""), "An ID string for this job",
                  Direction::Output);
}

void SubmitRemoteJob::exec() {
  // Put the algorithm execution code here...

  // The first thing to do will almost certainly be to retrieve the input
  // workspace.
  // Here's the line for that - just uncomment it:
  //   MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");

  boost::shared_ptr<RemoteJobManager> jobManager =
      ConfigService::Instance().getFacility().getRemoteJobManager(
          getPropertyValue("ComputeResource"));

  // jobManager is a boost::shared_ptr...
  if (!jobManager) {
    // Requested compute resource doesn't exist
    // TODO: should we create our own exception class for this??
    throw(std::runtime_error(
        std::string("Unable to create a compute resource named " +
                    getPropertyValue("ComputeResource"))));
  }

  RemoteJobManager::PostDataMap postData;

  postData["TransID"] = getPropertyValue("TransactionID");
  postData["NumNodes"] = getPropertyValue("NumNodes");
  postData["CoresPerNode"] = getPropertyValue("CoresPerNode");

  postData["ScriptName"] = getPropertyValue("ScriptName");
  postData[getPropertyValue("ScriptName")] = getPropertyValue("PythonScript");

  // Job name is optional
  std::string jobName = getPropertyValue("TaskName");
  if (jobName.length() > 0) {
    postData["JobName"] = jobName;
  }

  std::istream &respStream = jobManager->httpPost("/submit", postData);
  JSONObject resp;
  initFromStream(resp, respStream);
  if (jobManager->lastStatus() == Poco::Net::HTTPResponse::HTTP_CREATED) {
    std::string jobId;
    resp["JobID"].getValue(jobId);
    setPropertyValue("JobID", jobId);
    g_log.information() << "Job submitted.  Job ID =  "
                        << getPropertyValue("JobID") << std::endl;
  } else {
    std::string errMsg;
    resp["Err_Msg"].getValue(errMsg);
    throw(std::runtime_error(errMsg));
  }
}

} // end namespace RemoteAlgorithms
} // end namespace Mantid
