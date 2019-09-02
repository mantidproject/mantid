// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidRemoteAlgorithms/QueryAllRemoteJobs2.h"
#include "MantidAPI/RemoteJobManagerFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/NullValidator.h"

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
  declareProperty(std::make_unique<ArrayProperty<std::string>>(
                      "JobID", nullValidator, Direction::Output),
                  "ID string for the job(s)");
  declareProperty(std::make_unique<ArrayProperty<std::string>>(
                      "JobStatusString", nullValidator, Direction::Output),
                  "Description of the job's current status (Queued, Running, "
                  "Complete, etc..)");
  declareProperty(std::make_unique<ArrayProperty<std::string>>(
                      "JobName", nullValidator, Direction::Output),
                  "Name of the job (specified when the job was submitted)");
  declareProperty(std::make_unique<ArrayProperty<std::string>>(
                      "ScriptName", nullValidator, Direction::Output),
                  "The name of the script (python, etc.) or other type of "
                  "executable that the job runs");
  declareProperty(std::make_unique<ArrayProperty<std::string>>(
                      "TransID", nullValidator, Direction::Output),
                  "The ID of the transaction that owns the job");

  // Times for job submit, job start and job complete (may be empty depending
  // on the server-side implementation)
  declareProperty(std::make_unique<ArrayProperty<std::string>>(
                      "SubmitDate", nullValidator, Direction::Output),
                  "The date & time the job was submitted");
  declareProperty(std::make_unique<ArrayProperty<std::string>>(
                      "StartDate", nullValidator, Direction::Output),
                  "The date & time the job actually started executing");
  declareProperty(std::make_unique<ArrayProperty<std::string>>(
                      "CompletionDate", nullValidator, Direction::Output),
                  "The date & time the job finished");

  declareProperty(std::make_unique<ArrayProperty<std::string>>(
                      "CommandLine", nullValidator, Direction::Output),
                  "The command line run by this job on the remote compute "
                  "resource machine(s)");
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
  std::vector<std::string> cmdLine;
  for (auto &info : infos) {
    jobIds.push_back(info.id);
    jobNames.push_back(info.name);
    jobStatusStrs.push_back(info.status);
    transIds.push_back(info.transactionID);
    runNames.push_back(info.runnableName);
    submitDates.push_back(info.submitDate.toISO8601String());
    startDates.push_back(info.startDate.toISO8601String());
    completionDates.push_back(info.completionTime.toISO8601String());
    cmdLine.push_back(info.cmdLine);
  }
  setProperty("JobID", jobIds);
  setProperty("JobStatusString", jobStatusStrs);
  setProperty("JobName", jobNames);
  setProperty("ScriptName", runNames);
  setProperty("TransID", transIds);
  setProperty("SubmitDate", submitDates);
  setProperty("StartDate", startDates);
  setProperty("CompletionDate", completionDates);
  setProperty("CommandLine", cmdLine);
}

} // end namespace RemoteAlgorithms
} // end namespace Mantid
