#include "MantidAPI/RemoteJobManagerFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidRemoteAlgorithms/QueryRemoteFile2.h"

namespace Mantid {
namespace RemoteAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(QueryRemoteFile2)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

// A reference to the logger is provided by the base class, it is called g_log.

void QueryRemoteFile2::init() {
  // Unlike most algorithms, this one doesn't deal with workspaces....

  auto requireValue = boost::make_shared<MandatoryValidator<std::string>>();

  // Compute Resources
  std::vector<std::string> computes = Mantid::Kernel::ConfigService::Instance()
                                          .getFacility()
                                          .computeResources();
  declareProperty("ComputeResource", "",
                  boost::make_shared<StringListValidator>(computes),
                  "The name of the remote computer to query", Direction::Input);

  // The transaction ID comes from the StartRemoteTransaction algortithm
  declareProperty("TransactionID", "", requireValue,
                  "The ID of the transaction who's files we want to list",
                  Direction::Input);

  declareProperty(
      new ArrayProperty<std::string>("FileNames", Direction::Output),
      "The names of all the files that were found");
}

void QueryRemoteFile2::exec() {

  Mantid::API::IRemoteJobManager_sptr jm =
      Mantid::API::RemoteJobManagerFactory::Instance().create(
          getPropertyValue("ComputeResource"));

  std::string tid = getPropertyValue("TransactionID");
  std::vector<std::string> names = jm->queryRemoteFile(tid);

  setProperty("FileNames", names);
}

} // end namespace RemoteAlgorithms
} // end namespace Mantid
