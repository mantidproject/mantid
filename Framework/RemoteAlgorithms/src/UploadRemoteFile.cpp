#include "MantidRemoteAlgorithms/UploadRemoteFile.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/RemoteJobManager.h"
#include "MantidKernel/ListValidator.h"

#include "MantidRemoteAlgorithms/SimpleJSON.h"

#include "boost/make_shared.hpp"

#include <fstream>

namespace Mantid {
namespace RemoteAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(UploadRemoteFile)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

// A reference to the logger is provided by the base class, it is called g_log.

void UploadRemoteFile::init() {
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

void UploadRemoteFile::exec() {
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

  RemoteJobManager::PostDataMap postData;
  postData["TransID"] = getPropertyValue("TransactionID");

  std::string localFileName = getPropertyValue("LocalFileName");
  std::ifstream infile(localFileName.c_str());
  if (infile.good()) {
    // Yes, we're reading the entire file into memory.  Obviously, this is only
    // feasible for fairly small files...
    RemoteJobManager::PostDataMap fileData;
    fileData[getPropertyValue("RemoteFileName")] =
        std::string(std::istreambuf_iterator<char>(infile),
                    std::istreambuf_iterator<char>());
    infile.close();

    std::istream &respStream =
        jobManager->httpPost("/upload", postData, fileData);
    if (jobManager->lastStatus() ==
        Poco::Net::HTTPResponse::HTTP_CREATED) // Upload returns a "201 -
                                               // Created" code on success
    {
      g_log.information() << "Uploaded '" << getPropertyValue("RemoteFileName")
                          << "' to '" << getPropertyValue("LocalFileName")
                          << "'" << std::endl;
    } else {
      JSONObject resp;
      initFromStream(resp, respStream);
      std::string errMsg;
      resp["Err_Msg"].getValue(errMsg);
      throw(std::runtime_error(errMsg));
    }
  } else {
    throw(std::runtime_error(
        std::string("Failed to open " + getPropertyValue("LocalFileName"))));
  }
}

} // end namespace RemoteAlgorithms
} // end namespace Mantid
