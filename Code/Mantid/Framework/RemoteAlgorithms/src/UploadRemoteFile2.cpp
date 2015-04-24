#include "MantidAPI/RemoteJobManagerFactory.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidRemoteAlgorithms/UploadRemoteFile2.h"

namespace Mantid {
namespace RemoteAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(UploadRemoteFile2)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

// A reference to the logger is provided by the base class, it is called g_log.

void UploadRemoteFile2::init() {
  // Unlike most algorithms, this one doesn't deal with workspaces....

  auto requireValue = boost::make_shared<MandatoryValidator<std::string>>();

  // Compute Resources
  std::vector<std::string> computes = Mantid::Kernel::ConfigService::Instance()
                                          .getFacility()
                                          .computeResources();
  declareProperty("ComputeResource", "",
                  boost::make_shared<StringListValidator>(computes),
                  "The name of the remote computer to upload the file to",
                  Direction::Input);

  // The transaction ID comes from the StartRemoteTransaction algortithm
  declareProperty("TransactionID", "", requireValue,
                  "The transaction the file will be associated with",
                  Direction::Input);
  declareProperty("RemoteFileName", "", requireValue,
                  "The name to save the file as on the remote computer. "
                  "(Filename only; no path information)",
                  Direction::Input);
  declareProperty(
      "LocalFileName", "", requireValue,
      "The full pathname (on the local machine) of the file to upload",
      Direction::Input);
  // Note: 'RemoteFileName' is just the name.  The remote server figures out the
  // full path
  // from the transaction ID.  'LocalFileName' *IS* the full pathname (on the
  // local machine)
}

void UploadRemoteFile2::exec() {

  const std::string comp = getPropertyValue("ComputeResource");
  Mantid::API::IRemoteJobManager_sptr jobManager =
      Mantid::API::RemoteJobManagerFactory::Instance().create(
          comp);

  const std::string tid = getPropertyValue("TransactionID");
  const std::string remote = getPropertyValue("RemoteFileName");
  const std::string local = getPropertyValue("LocalFileName");
  jobManager->uploadRemoteFile(tid, remote, local);

  g_log.information() << "Uploaded '" << getPropertyValue("RemoteFileName")
                      << "' to '" << getPropertyValue("LocalFileName")
                      << "'" << " on the compute resource " << comp << std::endl;
}

} // end namespace RemoteAlgorithms
} // end namespace Mantid
