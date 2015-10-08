#include "MantidAPI/RemoteJobManagerFactory.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MaskedProperty.h"
#include "MantidRemoteAlgorithms/DownloadRemoteFile2.h"

namespace Mantid {
namespace RemoteAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(DownloadRemoteFile2)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

// A reference to the logger is provided by the base class, it is called g_log.

void DownloadRemoteFile2::init() {
  // Unlike most algorithms, this one doesn't deal with workspaces....

  auto requireValue = boost::make_shared<MandatoryValidator<std::string>>();

  // Compute Resources
  std::vector<std::string> computes = Mantid::Kernel::ConfigService::Instance()
                                          .getFacility()
                                          .computeResources();
  declareProperty(
      "ComputeResource", "", boost::make_shared<StringListValidator>(computes),
      "The name of the remote computer holding the file", Direction::Input);

  // The transaction ID comes from the StartRemoteTransaction algortithm
  declareProperty("TransactionID", "", requireValue,
                  "The ID of the transaction that owns the file",
                  Direction::Input);
  declareProperty(
      "RemoteFileName", "", requireValue,
      "The name of the file on the remote machine. (Filename only; no path)",
      Direction::Input);
  declareProperty("LocalFileName", "", requireValue,
                  "The full pathname on the local machine where the downloaded "
                  "file should be saved.",
                  Direction::Input);
  // Note: 'RemoteFileName' is just the name.  The remote server figures out the
  // full path
  // from the transaction ID.  'LocalFileName' *IS* the full pathname (on the
  // local machine)
}

void DownloadRemoteFile2::exec() {

  Mantid::API::IRemoteJobManager_sptr jobManager =
      Mantid::API::RemoteJobManagerFactory::Instance().create(
          getPropertyValue("ComputeResource"));

  const std::string tid = getPropertyValue("TransactionID");
  const std::string remote = getPropertyValue("RemoteFileName");
  const std::string local = getPropertyValue("LocalFileName");
  jobManager->downloadRemoteFile(tid, remote, local);

  g_log.information() << "Downloaded '" << remote << "' to '" << local << "'"
                      << std::endl;
}

} // end namespace RemoteAlgorithms
} // end namespace Mantid
