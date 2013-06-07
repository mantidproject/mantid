#include "MantidRemoteAlgorithms/QueryRemoteFile.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MaskedProperty.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"

#include "MantidRemote/RemoteJobManager.h"

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

  // TODO: Can we figure out the user name/group name automatically?
  declareProperty( "UserName", "", requireValue, "", Direction::Input);

  // Password doesn't get echoed to the screen...
  declareProperty( new MaskedProperty<std::string>( "Password", "", requireValue, Direction::Input), "");


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

  // Set the username and password from the properties
  jobManager->setUserName( getPropertyValue ("UserName"));
  jobManager->setPassword( getPropertyValue( "Password"));

  std::vector <std::string> files;
  std::string errMsg;
  if (jobManager->listFiles( getPropertyValue("TransactionID"), files, errMsg) == RemoteJobManager::JM_OK)
  {
    setProperty( "FileNames", files);
  }
  else
  {
    throw( std::runtime_error( "Error listing remote files: " + errMsg));
  }
}

} // end namespace RemoteAlgorithms
} // end namespace Mantid