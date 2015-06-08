#include "MantidAPI/RemoteJobManagerFactory.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/NullValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidRemoteAlgorithms/QueryRemoteJob2.h"

namespace Mantid {
namespace RemoteAlgorithms {

// Register the algorithm into the Algorithm Factory
DECLARE_ALGORITHM(QueryRemoteJob2)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

// A reference to the logger is provided by the base class, it is called g_log.

void QueryRemoteJob2::init() {
  // Unlike most algorithms, this one doesn't deal with workspaces....

  auto requireValue = boost::make_shared<MandatoryValidator<std::string>>();
  auto nullValidator = boost::make_shared<NullValidator>();

  // Compute Resources
  std::vector<std::string> computes = Mantid::Kernel::ConfigService::Instance()
                                          .getFacility()
                                          .computeResources();
  declareProperty(
      "ComputeResource", "", boost::make_shared<StringListValidator>(computes),
      "The name of the remote compute resource to query", Direction::Input);

  // The ID of the job we want to query
  declareProperty("JobID", "", requireValue, "The ID of the job to query",
                  Direction::Input);

  // Name given to the job
  declareProperty("JobName", "", nullValidator, "The name of the job",
                  Direction::Output);

  // Name of the executable/python or other kind of script that was (or will be)
  // run
  declareProperty(
      "ScriptName", "", nullValidator,
      "The name of the script or executable that was (or will be) run",
      Direction::Output);

  // A human readable description of the job's status
  declareProperty("JobStatusString", "", nullValidator,
                  "The current status of the job (example: Queued, Running, "
                  "Complete, etc..)",
                  Direction::Output);

  // Transaction ID this job is associated with
  declareProperty("TransID", "", nullValidator,
                  "The transaction ID this job was submitted under",
                  Direction::Output);

  // Dates and times for job submit, job start and job complete (may be empty
  // depending on the server-side implementation)
  declareProperty("SubmitDate", "", nullValidator,
                  "The date & time the job was submitted (availability is "
                  "optional and implementation dependent)",
                  Direction::Output);
  declareProperty("StartDate", "", nullValidator,
                  "The date & time the job actually started executing "
                  "(availability is optional and implementation dependent)",
                  Direction::Output);
  declareProperty("CompletionDate", "", nullValidator,
                  "The date & time the job finished (availability is optional "
                  "and implementation dependent)",
                  Direction::Output);
}

void QueryRemoteJob2::exec() {
  Mantid::API::IRemoteJobManager_sptr jm =
      Mantid::API::RemoteJobManagerFactory::Instance().create(
          getPropertyValue("ComputeResource"));

  Mantid::API::IRemoteJobManager::RemoteJobInfo info =
      jm->queryRemoteJob(getPropertyValue("JobID"));

  setProperty("JobName", info.name);
  setProperty("ScriptName", info.runnableName);
  setProperty("JobStatusString", info.status);
  setProperty("TransID", info.transactionID);
  setProperty("SubmitDate", info.submitDate.toISO8601String());
  setProperty("StartDate", info.startDate.toISO8601String());
  setProperty("CompletionDate", info.completionTime.toISO8601String());
}

} // end namespace RemoteAlgorithms
} // end namespace Mantid
