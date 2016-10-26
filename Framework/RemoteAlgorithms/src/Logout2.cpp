#include "MantidAPI/RemoteJobManagerFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidRemoteAlgorithms/Logout2.h"

namespace Mantid {
namespace RemoteAlgorithms {

// Register the algorithm into the Algorithm Factory
DECLARE_ALGORITHM(Logout2)

using namespace Mantid::Kernel;

// A reference to the logger is provided by the base class, it is called g_log.

void Logout2::init() {
  // Unlike most algorithms, this wone doesn't deal with workspaces....

  auto requireValue = boost::make_shared<MandatoryValidator<std::string>>();

  // Compute Resources
  std::vector<std::string> computes = Mantid::Kernel::ConfigService::Instance()
                                          .getFacility()
                                          .computeResources();
  declareProperty("ComputeResource", "",
                  boost::make_shared<StringListValidator>(computes),
                  "The remote computer to log out from", Direction::Input);

  declareProperty("UserName", "", requireValue,
                  "Name of the user to authenticate as", Direction::Input);
}

void Logout2::exec() {

  const std::string comp = getPropertyValue("ComputeResource");
  Mantid::API::IRemoteJobManager_sptr jobManager =
      Mantid::API::RemoteJobManagerFactory::Instance().create(comp);

  const std::string user = getPropertyValue("UserName");
  jobManager->logout(user);

  g_log.information() << "Logged out from the compute resource " << comp
                      << ". You will need to authenticate before interacting "
                         "again with the resource. \n";
}

} // end namespace RemoteAlgorithms
} // end namespace Mantid
