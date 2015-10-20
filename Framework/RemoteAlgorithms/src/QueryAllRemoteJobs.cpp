#include "MantidRemoteAlgorithms/QueryAllRemoteJobs.h"
#include "MantidKernel/NullValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"

#include "MantidKernel/RemoteJobManager.h"
#include "MantidRemoteAlgorithms/SimpleJSON.h"

#include "boost/make_shared.hpp"

namespace Mantid {
namespace RemoteAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(QueryAllRemoteJobs)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

// A reference to the logger is provided by the base class, it is called g_log.

void QueryAllRemoteJobs::init() {
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
  // declare several
  // array properties for different pieces of data.  Values from the same array
  // index are for
  // the same job.
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
                  "The name of the python script that was executed");
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

void QueryAllRemoteJobs::exec() {
  boost::shared_ptr<RemoteJobManager> jobManager =
      Mantid::Kernel::ConfigService::Instance()
          .getFacility()
          .getRemoteJobManager(getPropertyValue("ComputeResource"));

  // jobManager is a boost::shared_ptr...
  if (!jobManager) {
    // Requested compute resource doesn't exist
    // TODO: should we create our own exception class for this??
    throw(std::runtime_error(
        std::string("Unable to create a compute resource named " +
                    getPropertyValue("ComputeResource"))));
  }

  std::istream &respStream = jobManager->httpGet("/query");
  JSONObject resp;
  try {
    initFromStream(resp, respStream);
  } catch (JSONParseException &) {
    // Nobody else knows what a JSONParseException is, so rethrow as a
    // runtime_error
    throw(std::runtime_error("Error parsing data returned from the server.  "
                             "This probably indicates a server-side error of "
                             "some kind."));
  }

  if (jobManager->lastStatus() == Poco::Net::HTTPResponse::HTTP_OK) {
    std::vector<std::string> jobIds;
    std::vector<std::string> jobStatusStrs;
    std::vector<std::string> jobNames;
    std::vector<std::string> scriptNames;
    std::vector<std::string> transIds;
    std::vector<std::string> submitDates;
    std::vector<std::string> startDates;
    std::vector<std::string> completionDates;

    JSONObject::const_iterator it = resp.begin();
    while (it != resp.end()) {
      jobIds.push_back((*it).first);
      JSONObject jobData;
      (*it).second.getValue(jobData);

      std::string value;
      jobData["JobStatus"].getValue(value);
      jobStatusStrs.push_back(value);

      jobData["JobName"].getValue(value);
      jobNames.push_back(value);

      jobData["ScriptName"].getValue(value);
      scriptNames.push_back(value);

      jobData["TransID"].getValue(value);
      transIds.push_back(value);

      // The time stuff is actually an optional extension.  We could check the
      // info
      // URL and see if the server implements it, but it's easier to just look
      // in
      // the output and see if the values are there...
      if (jobData.find("SubmitDate") != jobData.end()) {
        jobData["SubmitDate"].getValue(value);
        submitDates.push_back(value);

        jobData["StartDate"].getValue(value);
        startDates.push_back(value);

        jobData["CompletionDate"].getValue(value);
        completionDates.push_back(value);
      } else {
        // push back empty strings just so all the array properties have the
        // same
        // number of elements
        submitDates.push_back("");
        startDates.push_back("");
        completionDates.push_back("");
      }

      ++it;
    }

    setProperty("JobId", jobIds);
    setProperty("JobStatusString", jobStatusStrs);
    setProperty("JobName", jobNames);
    setProperty("ScriptName", scriptNames);
    setProperty("TransID", transIds);
    setProperty("SubmitDate", submitDates);
    setProperty("StartDate", startDates);
    setProperty("CompletionDate", completionDates);

  } else {
    std::string errMsg;
    resp["Err_Msg"].getValue(errMsg);
    throw(std::runtime_error(errMsg));
  }
}

} // end namespace RemoteAlgorithms
} // end namespace Mantid
