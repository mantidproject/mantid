#include "MantidRemoteAlgorithms/DownloadRemoteFile.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/MaskedProperty.h"
#include "MantidKernel/RemoteJobManager.h"
#include "MantidKernel/ListValidator.h"
#include "MantidRemoteAlgorithms/SimpleJSON.h"

#include "boost/make_shared.hpp"

#include <fstream>

namespace Mantid {
namespace RemoteAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(DownloadRemoteFile)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

// A reference to the logger is provided by the base class, it is called g_log.

void DownloadRemoteFile::init() {
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

void DownloadRemoteFile::exec() {
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
      "/download", std::string("TransID=") + getPropertyValue("TransactionID") +
                       "&File=" + getPropertyValue("RemoteFileName"));

  if (jobManager->lastStatus() == Poco::Net::HTTPResponse::HTTP_OK) {

    std::string localFileName = getPropertyValue("LocalFileName");
    std::ofstream outfile(localFileName.c_str());
    if (outfile.good()) {
      outfile << respStream.rdbuf();
      outfile.close();
      g_log.information() << "Downloaded '"
                          << getPropertyValue("RemoteFileName") << "' to '"
                          << getPropertyValue("LocalFileName") << "'"
                          << std::endl;
    } else {
      throw(std::runtime_error(
          std::string("Failed to open " + getPropertyValue("LocalFileName"))));
    }
  } else {
    JSONObject resp;
    initFromStream(resp, respStream);
    std::string errMsg;
    resp["Err_Msg"].getValue(errMsg);
    throw(std::runtime_error(errMsg));
  }
}

} // end namespace RemoteAlgorithms
} // end namespace Mantid
