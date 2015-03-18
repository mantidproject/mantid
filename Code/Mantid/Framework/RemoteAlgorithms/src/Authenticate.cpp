#include "MantidRemoteAlgorithms/Authenticate.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/MaskedProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/RemoteJobManager.h"

#include "MantidRemoteAlgorithms/SimpleJSON.h"

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

namespace Mantid {
namespace RemoteAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Authenticate)

using namespace Mantid::Kernel;
// using namespace Mantid::API;
// using namespace Mantid::Geometry;

// A reference to the logger is provided by the base class, it is called g_log.

void Authenticate::init() {
  // Unlike most algorithms, this wone doesn't deal with workspaces....

  auto requireValue = boost::make_shared<MandatoryValidator<std::string>>();

  // Compute Resources
  std::vector<std::string> computes = Mantid::Kernel::ConfigService::Instance()
                                          .getFacility()
                                          .computeResources();
  declareProperty("ComputeResource", "",
                  boost::make_shared<StringListValidator>(computes),
                  "The remote computer to authenticate to", Direction::Input);

  // Say who we are (or at least, who we want to execute the remote python code)
  declareProperty("UserName", "", requireValue,
                  "Name of the user to authenticate as", Direction::Input);

  // Password doesn't get echoed to the screen...
  declareProperty(new MaskedProperty<std::string>("Password", "", requireValue,
                                                  Direction::Input),
                  "The password associated with the specified user");
}

void Authenticate::exec() {
  boost::shared_ptr<RemoteJobManager> jobManager =
      ConfigService::Instance().getFacility().getRemoteJobManager(
          getPropertyValue("ComputeResource"));

  // jobManager is a boost::shared_ptr...
  if (!jobManager) {
    // Requested compute resource doesn't exist
    // TODO: should we create our own exception class for this??
    throw(std::runtime_error(
        std::string("Unknown create a compute resource named " +
                    getPropertyValue("ComputeResource"))));
  }

  std::istream &respStream =
      jobManager->httpGet("/authenticate", "", getPropertyValue("UserName"),
                          getPropertyValue("Password"));
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
