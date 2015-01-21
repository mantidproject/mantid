#include "MantidRemoteAlgorithms/AbortRemoteJob.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/NullValidator.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"
#include "MantidRemoteAlgorithms/SimpleJSON.h"
#include "MantidKernel/RemoteJobManager.h"

#include "boost/make_shared.hpp"

namespace Mantid {
namespace RemoteAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AbortRemoteJob)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

// A reference to the logger is provided by the base class, it is called g_log.

void AbortRemoteJob::init() {
  // Unlike most algorithms, this one doesn't deal with workspaces....

  auto requireValue = boost::make_shared<MandatoryValidator<std::string>>();
  auto nullValidator = boost::make_shared<NullValidator>();

  // Compute Resources
  std::vector<std::string> computes = Mantid::Kernel::ConfigService::Instance()
                                          .getFacility()
                                          .computeResources();
  declareProperty(
      "ComputeResource", "", boost::make_shared<StringListValidator>(computes),
      "The remote computer where the job is running", Direction::Input);

  // The ID of the job we want to Abort
  declareProperty("JobID", "", requireValue, "The ID of the job to abort",
                  Direction::Input);
}

void AbortRemoteJob::exec() {
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

  std::istream &respStream = jobManager->httpGet(
      "/abort", std::string("JobID=") + getPropertyValue("JobID"));

  if (jobManager->lastStatus() != Poco::Net::HTTPResponse::HTTP_OK) {
    JSONObject resp;
    initFromStream(resp, respStream);
    std::string errMsg;
    resp["Err_Msg"].getValue(errMsg);
    throw(std::runtime_error(errMsg));
  }
}

} // end namespace RemoteAlgorithms
} // end namespace Mantid
