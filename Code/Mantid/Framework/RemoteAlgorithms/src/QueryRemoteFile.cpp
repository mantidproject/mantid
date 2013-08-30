#include "MantidRemoteAlgorithms/QueryRemoteFile.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"

#include "MantidKernel/RemoteJobManager.h"
#include "MantidRemote/SimpleJSON.h"

#include "boost/make_shared.hpp"

namespace Mantid
{
namespace RemoteAlgorithms
{
    
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(QueryRemoteFile)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages

void QueryRemoteFile::init()
{
  // Unlike most algorithms, this one doesn't deal with workspaces....

  auto requireValue = boost::make_shared<MandatoryValidator<std::string> >();

  // Compute Resources
  std::vector<std::string> computes = Mantid::Kernel::ConfigService::Instance().getFacility().computeResources();
  declareProperty( "ComputeResource", "", boost::make_shared<StringListValidator>(computes), "", Direction::Input);

  // The transaction ID comes from the StartRemoteTransaction algortithm
  declareProperty( "TransactionID", "", requireValue, "", Direction::Input);

  declareProperty(new ArrayProperty<std::string>( "FileNames", Direction::Output));

}

void QueryRemoteFile::exec()
{
  boost::shared_ptr<RemoteJobManager> jobManager = Mantid::Kernel::ConfigService::Instance().getFacility().getRemoteJobManager( getPropertyValue("ComputeResource"));

  // jobManager is a boost::shared_ptr...
  if (! jobManager)
  {
    // Requested compute resource doesn't exist
    // TODO: should we create our own exception class for this??
    throw( std::runtime_error( std::string("Unable to create a compute resource named " + getPropertyValue("ComputeResource"))));
  }

  std::istream &respStream = jobManager->httpGet("/files", std::string("TransID=") + getPropertyValue("TransactionID"));
  JSONObject resp;
  initFromStream( resp, respStream);
  if (jobManager->lastStatus() == Poco::Net::HTTPResponse::HTTP_OK)
  {

    JSONArray files;
    std::vector<std::string> filenames;
    std::string oneFile;
    resp["Files"].getValue( files);
    for (unsigned int i = 0; i < files.size(); i++)
    {
      files[i].getValue( oneFile);
      filenames.push_back(oneFile);
    }

    setProperty( "FileNames", filenames);
  }
  else
  {
    std::string errMsg;
    resp["Err_Msg"].getValue( errMsg);
    throw( std::runtime_error( errMsg));
  }

}

} // end namespace RemoteAlgorithms
} // end namespace Mantid
