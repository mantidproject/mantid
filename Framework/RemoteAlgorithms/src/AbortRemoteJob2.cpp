#include "MantidAPI/RemoteJobManagerFactory.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/NullValidator.h"
#include "MantidRemoteAlgorithms/AbortRemoteJob2.h"

namespace Mantid {
namespace RemoteAlgorithms {

// Register the algorithm into the Algorithm Factory
DECLARE_ALGORITHM(AbortRemoteJob2)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

// A reference to the logger is provided by the base class, it is called g_log.

void AbortRemoteJob2::init() {
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

void AbortRemoteJob2::exec() {

  const std::string comp = getPropertyValue("ComputeResource");
  Mantid::API::IRemoteJobManager_sptr jobManager =
      Mantid::API::RemoteJobManagerFactory::Instance().create(comp);

  std::string jid = getPropertyValue("JobID");
  jobManager->abortRemoteJob(jid);
  g_log.information() << "Aborted job with ID " << jid
                      << " on the compute resource" << comp << std::endl;
}

} // namespace RemoteAlgorithms
} // namespace Mantid
