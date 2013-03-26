#include "MantidAlgorithms/StartRemoteTransaction.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/FacilityInfo.h"

#include "MantidRemote/RemoteJobManager.h"

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(StartRemoteTransaction)

using namespace Mantid::Kernel;

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages

void StartRemoteTransaction::init()
{
  auto requireValue = boost::make_shared<Mantid::Kernel::MandatoryValidator<std::string> >();

  // TODO: need a better validator for the compute facility property
  declareProperty( "ComputeResource", "", requireValue, "", Mantid::Kernel::Direction::Input);

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

