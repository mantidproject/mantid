#include "MantidAPI/RemoteJobManagerFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/NullValidator.h"
#include "MantidRemoteAlgorithms/QueryAllRemoteJobs2.h"

namespace Mantid {
namespace RemoteAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(QueryAllRemoteJobs2)

using namespace Mantid::Kernel;

// A reference to the logger is provided by the base class, it is called g_log.

void QueryAllRemoteJobs2::init() {
  // Unlike most algorithms, this one doesn't deal with workspaces....

  auto nullValidator = boost::make_shared<NullValidator>();

  // Compute Resources
  std::vector<std::string> computes = Mantid::Kernel::ConfigService::Instance()
                                          .getFacility()
                                          .computeResources();
  declareProperty("ComputeResource", "",
                  boost::make_shared<StringListValidator>(computes),
                  "The name of the remote computer to query", Direction::Input);

  // Mantid can't store arbitrary structs in its properties, so we're going to
  // declare several array properties for different pieces of data.  Values from
  // the same array index are for the same job.
  declareProperty(
      new ArrayProperty<std::string>("JobId", nullValidator, Direction::Output),
      "ID string for the job");
  declareProperty(new ArrayProperty<std::string>(
                      "JobStatusString", nullValidator, Direction::Output),
                  "Description of the job's current status (Queued, Running, "
                  "Complete, etc..)");
  declareProperty(new ArrayProperty<std::string>("JobName", nullValidator,
                                                 Direction::Output),
                  "Name of the job (specified when the job was submitted)");
  declareProperty(new ArrayProperty<std::string>("ScriptName", nullValidator,
                                                 Direction::Output),
                  "The name of the script (python, etc.) or other type of "
                  "executable that the job runs");
  declareProperty(new ArrayProperty<std::string>("TransID", nullValidator,
                                                 Direction::Output),
                  "The ID of the transaction that owns the job");

  // Times for job submit, job start and job complete (may be empty depending
  // on the server-side implementation)
  declareProperty(new ArrayProperty<std::string>("SubmitDate", nullValidator,
                                                 Direction::Output),
                  "The date & time the job was submitted");
  declareProperty(new ArrayProperty<std::string>("StartDate", nullValidator,
                                                 Direction::Output),
                  "The date & time the job actually started executing");
  declareProperty(new ArrayProperty<std::string>(
                      "CompletionDate", nullValidator, Direction::Output),
                  "The date & time the job finished");
}

void QueryAllRemoteJobs2::exec() {
  Mantid::API::IRemoteJobManager_sptr jm =
      Mantid::API::RemoteJobManagerFactory::Instance().create(
          getPropertyValue("ComputeResource"));

  std::vector<Mantid::API::IRemoteJobManager::RemoteJobInfo> infos =
      jm->queryAllRemoteJobs();

  std::vector<std::string> jobIds;
  std::vector<std::string> jobStatusStrs;
  std::vector<std::string> jobNames;
  std::vector<std::string> runNames;
  std::vector<std::string> transIds;
  std::vector<std::string> submitDates;
  std::vector<std::string> startDates;
  std::vector<std::string> completionDates;
  for (size_t j = 0; j < infos.size(); ++j) {
    jobIds.push_back(infos[j].id);
    jobNames.push_back(infos[j].name);
    jobStatusStrs.push_back(infos[j].status);
    transIds.push_back(infos[j].transactionID);
    runNames.push_back(infos[j].runnableName);
    submitDates.push_back(infos[j].submitDate.toISO8601String());
    startDates.push_back(infos[j].startDate.toISO8601String());
    completionDates.push_back(infos[j].completionTime.toISO8601String());
  }
  setProperty("JobId", jobIds);
  setProperty("JobStatusString", jobStatusStrs);
  setProperty("JobName", jobNames);
  setProperty("ScriptName", runNames);
  setProperty("TransID", transIds);
  setProperty("SubmitDate", submitDates);
  setProperty("StartDate", startDates);
  setProperty("CompletionDate", completionDates);
}

} // end namespace RemoteAlgorithms
} // end namespace Mantid
