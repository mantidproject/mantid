#include "MantidRemoteAlgorithms/DownloadRemoteFile.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/MaskedProperty.h"
#include "MantidRemote/RemoteJobManager.h"
#include "MantidKernel/ListValidator.h"

#include "boost/make_shared.hpp"

namespace Mantid
{
namespace RemoteAlgorithms
{
    
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(DownloadRemoteFile)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages

void DownloadRemoteFile::init()
{
  // Unlike most algorithms, this one doesn't deal with workspaces....

  auto requireValue = boost::make_shared<MandatoryValidator<std::string> >();

  // Compute Resources
  std::vector<std::string> computes = Mantid::Kernel::ConfigService::Instance().getFacility().computeResources();
  declareProperty( "ComputeResource", "", boost::make_shared<StringListValidator>(computes), "", Direction::Input);

  // TODO: Can we figure out the user name/group name automatically?
  declareProperty( "UserName", "", requireValue, "", Direction::Input);

  // Password doesn't get echoed to the screen...
  declareProperty( new MaskedProperty<std::string>( "Password", "", requireValue, Direction::Input), "");

  // The transaction ID comes from the StartRemoteTransaction algortithm
  declareProperty( "TransactionID", "", requireValue, "", Direction::Input);
  declareProperty( "RemoteFileName", "", requireValue, "", Direction::Input);
  declareProperty( "LocalFileName", "", requireValue, "", Direction::Input);
  // Note: 'RemoteFileName' is just the name.  The remote server figures out the full path
  // from the transaction ID.  'LocalFileName' *IS* the full pathname (on the local machine)

}

void DownloadRemoteFile::exec()
{
  boost::shared_ptr<RemoteJobManager> jobManager = Mantid::Kernel::ConfigService::Instance().getFacility().getRemoteJobManager( getPropertyValue("ComputeResource"));

  // jobManager is a boost::shared_ptr...
  if (! jobManager)
  {
    // Requested compute resource doesn't exist
    // TODO: should we create our own exception class for this??
    throw( std::runtime_error( std::string("Unable to create a compute resource named " + getPropertyValue("ComputeResource"))));
  }

  // Set the username and password from the properties
  jobManager->setUserName( getPropertyValue ("UserName"));
  jobManager->setPassword( getPropertyValue( "Password"));

  std::string errMsg;
  if (jobManager->downloadFile( getPropertyValue("TransactionID"), getPropertyValue("RemoteFileName"),
                                getPropertyValue("LocalFileName"), errMsg) != RemoteJobManager::JM_OK)
  {
    throw( std::runtime_error( "Error downloading remote file: " + errMsg));
  }

}

} // end namespace RemoteAlgorithms
} // end namespace Mantid