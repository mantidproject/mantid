#include "MantidRemoteAlgorithms/StartRemoteTransaction.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"

#include "MantidRemote/RemoteJobManager.h"

#include "boost/make_shared.hpp"

namespace Mantid
{
namespace RemoteAlgorithms
{
    
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(StartRemoteTransaction)

using namespace Mantid::Kernel;

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages

void StartRemoteTransaction::init()
{
  auto requireValue = boost::make_shared<Mantid::Kernel::MandatoryValidator<std::string> >();

  // Compute Resources
  std::vector<std::string> computes = Mantid::Kernel::ConfigService::Instance().getFacility().computeResources();
  declareProperty( "ComputeResource", "", boost::make_shared<StringListValidator>(computes), "", Direction::Input);

  // Two output properties
  declareProperty( "TransactionID", "", Direction::Output);
  declareProperty( "TempDirectory", "", Direction::Output);  // directory created on the server for this transaction's use
}

void StartRemoteTransaction::exec()
{

  boost::shared_ptr<RemoteJobManager> jobManager = Mantid::Kernel::ConfigService::Instance().getFacility().getRemoteJobManager( getPropertyValue("ComputeResource"));

  // jobManager is a boost::shared_ptr...
  if (! jobManager)
  {
    // Requested compute resource doesn't exist
    // TODO: should we create our own exception class for this??
    throw( std::runtime_error( std::string("Unable to create a compute resource named " + getPropertyValue("ComputeResource"))));
  }

  std::string transId;
  std::string tempDir;
  std::string errMsg;

  if (jobManager->startTransaction( transId, tempDir, errMsg) == RemoteJobManager::JM_OK)
  {
    setPropertyValue( "TransactionID", transId);
    setPropertyValue( "TempDirectory", tempDir);

    g_log.information() << "Transaction ID: " << transId << std::endl;
    g_log.information() << "Temporary directory: " << tempDir << std::endl;
  }
  else
  {
    throw( std::runtime_error( "Error starting transaction: " + errMsg));
  }
}

} // end namespace RemoteAlgorithms
} // end namespace Mantid