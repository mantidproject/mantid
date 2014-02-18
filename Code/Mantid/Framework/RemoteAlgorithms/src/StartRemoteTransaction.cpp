/*WIKI*

Start a job transaction on a remote compute resource.

For more details, see the [[Remote_Job_Submission_API|remote job submission API docs]].

*WIKI*/

#include "MantidRemoteAlgorithms/StartRemoteTransaction.h"
#include "MantidRemoteAlgorithms/SimpleJSON.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"

#include "MantidKernel/RemoteJobManager.h"

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

void StartRemoteTransaction::initDocs()
{
  this->setWikiSummary("Start a job transaction on a remote compute resource.");
  this->setOptionalMessage("Start a job transaction on a remote compute resource.");
}

void StartRemoteTransaction::init()
{
  // Compute Resources
  std::vector<std::string> computes = Mantid::Kernel::ConfigService::Instance().getFacility().computeResources();
  declareProperty( "ComputeResource", "", boost::make_shared<StringListValidator>(computes), "The name of the remote computer where the new transaction will be created", Direction::Input);

  // output property
  declareProperty( "TransactionID", std::string(""), "The ID of the new transaction", Direction::Output);
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

  std::istream &respStream = jobManager->httpGet( "/transaction", "Action=Start");
  JSONObject resp;
  initFromStream( resp, respStream);

  if ( jobManager->lastStatus() == Poco::Net::HTTPResponse::HTTP_OK)
  {
    std::string transId;
    resp["TransID"].getValue( transId);
    setPropertyValue( "TransactionID", transId);
    g_log.information() << "Transaction ID " << transId << " started." << std::endl;
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
