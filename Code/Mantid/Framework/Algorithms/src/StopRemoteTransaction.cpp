#include "MantidAlgorithms/StopRemoteTransaction.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidRemote/RemoteJobManager.h"
#include "MantidKernel/FacilityInfo.h"

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(StopRemoteTransaction)

using namespace Mantid::Kernel;

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages

void StopRemoteTransaction::init()
{
  auto requireValue = boost::make_shared<Mantid::Kernel::MandatoryValidator<std::string> >();

  // TODO: need a better validator for the compute facility property
  declareProperty( "ComputeResource", "", requireValue, "", Mantid::Kernel::Direction::Input);

  // The transaction ID comes from the StartRemoteTransaction algortithm
  declareProperty( "TransactionID", "", requireValue, "", Mantid::Kernel::Direction::Input);
}

void StopRemoteTransaction::exec()
{
  boost::shared_ptr<RemoteJobManager> jobManager = Mantid::Kernel::ConfigService::Instance().getFacility().getRemoteJobManager( getPropertyValue("ComputeResource"));

  // jobManager is a boost::shared_ptr...
  if (! jobManager)
  {
    // Requested compute resource doesn't exist
    // TODO: should we create our own exception class for this??
    throw( std::runtime_error( std::string("Unable to create a compute resource named " + getPropertyValue("ComputeResource"))));
  }

  std::string errMsg;
  std::string transId = getPropertyValue( "TransactionID");

  if (jobManager->stopTransaction( transId, errMsg) == RemoteJobManager::JM_OK)
  {
    g_log.information() << "Transaction ID " << transId << " stopped." << std::endl;
  }
  else
  {
    throw( std::runtime_error( "Error stopping transaction " + transId + ": " + errMsg));
  }

}

