//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraAxis.h"
#include <numeric>

namespace Mantid
{
namespace API
{
namespace OperatorOverloads
{

  /** Performs a binary operation on two workspaces
   *  @param algorithmName :: The name of the binary operation to perform
   *  @param lhs :: left hand side workspace shared pointer
   *  @param rhs :: right hand side workspace shared pointer
   *  @param lhsAsOutput :: If true, indicates that the lhs input is the same workspace as the output one
   *  @param child :: If true the algorithm is run as a child
   *  @param name :: If child is true and this is not an inplace operation then this name is used as output
   *  @param A :: flag indicating whether to rethrow exceptions
   *  @return The result in a workspace shared pointer
   */
  MatrixWorkspace_sptr executeBinaryOperation(const std::string & algorithmName, const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs, bool lhsAsOutput, bool child, const std::string &name, bool rethrow)
  {
    IAlgorithm_sptr alg = AlgorithmManager::Instance().createUnmanaged(algorithmName);
    alg->setChild(child);
    alg->setRethrows(rethrow);
    alg->initialize();

    if( child )
    {
      alg->setProperty<MatrixWorkspace_sptr>("LHSWorkspace",lhs);
      alg->setProperty<MatrixWorkspace_sptr>("RHSWorkspace",rhs);
      alg->setProperty<bool>("AllowDifferentNumberSpectra",false);
      // Have to set a text name for the output workspace if the algorithm is a child even
      // though it will not be used.
      alg->setPropertyValue("OutputWorkspace","��NotApplicable");
      if( lhsAsOutput )
      {
        alg->setProperty<MatrixWorkspace_sptr>("OutputWorkspace",lhs);
      }
    }
    // If this is not a child algorithm then we need names for the properties
    else
    {
      alg->setPropertyValue("LHSWorkspace",lhs->getName());
      alg->setPropertyValue("RHSWorkspace",rhs->getName());
      alg->setProperty<bool>("AllowDifferentNumberSpectra",false);
      if( lhsAsOutput )
      {
        alg->setPropertyValue("OutputWorkspace",lhs->getName());
      }
      else
      {
        alg->setPropertyValue("OutputWorkspace",name);
      }
    }

    alg->execute();

    if (alg->isExecuted())
    {
      // Get the output workspace property
      if( child )
      {
        return alg->getProperty("OutputWorkspace");
      }
      else
      {
        API::Workspace_sptr result =
            API::AnalysisDataService::Instance().retrieve(alg->getPropertyValue("OutputWorkspace"));
        return boost::dynamic_pointer_cast<API::MatrixWorkspace>(result);
      }
    }
    else
    {
      std::string message = "Error while executing operation: " + algorithmName;
      throw std::runtime_error(message);
    }

    throw Kernel::Exception::NotFoundError("Required output workspace property not found on sub algorithm" ,"OutputWorkspace");

    //Horendous code inclusion to satisfy compilers that all code paths return a value
    // in reality the above code should either throw or return successfully.
    return MatrixWorkspace_sptr();
  }





} // namespace OperatorOverloads



/** Performs a comparison operation on two workspaces, using the CheckWorkspacesMatch algorithm
 *
 *  @param lhs :: left hand side workspace shared pointer
 *  @param rhs :: right hand side workspace shared pointer
 *  @return bool, true if workspaces match
 */
bool equals(const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs, double tolerance)
{
  IAlgorithm_sptr alg = AlgorithmManager::Instance().createUnmanaged("CheckWorkspacesMatch");
  alg->setChild(true);
  alg->setRethrows(false);
  alg->initialize();
  alg->setProperty<MatrixWorkspace_sptr>("Workspace1",lhs);
  alg->setProperty<MatrixWorkspace_sptr>("Workspace2",rhs);
  alg->setProperty<MatrixWorkspace_sptr>("Workspace2",rhs);
  alg->setProperty<double>("Tolerance", tolerance);
  // Rest: use default

  alg->execute();
  if (alg->isExecuted())
  {
    return (alg->getPropertyValue("Result") == "Success!");
  }
  else
  {
    std::string message = "Error while executing operation: CheckWorkspacesMatch";
    throw std::runtime_error(message);
  }
  return false;
}




/** Creates a temporary single value workspace the error is set to zero
 *  @param rhsValue :: the value to use
 *  @return The value in a workspace shared pointer
 */
static MatrixWorkspace_sptr createWorkspaceSingleValue(const double& rhsValue)
{
  MatrixWorkspace_sptr retVal = WorkspaceFactory::Instance().create("WorkspaceSingleValue",1,1,1);
  retVal->dataY(0)[0]=rhsValue;

  return retVal;
}

using OperatorOverloads::executeBinaryOperation;

/** Adds two workspaces
 *  @param lhs :: left hand side workspace shared pointer
 *  @param rhs :: right hand side workspace shared pointer
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator+(const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs)
{
  return executeBinaryOperation("Plus",lhs,rhs);
}

/** Adds a workspace to a single value
 *  @param lhs ::      left hand side workspace shared pointer
 *  @param rhsValue :: the single value
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator+(const MatrixWorkspace_sptr lhs, const double& rhsValue)
{
  return executeBinaryOperation("Plus",lhs,createWorkspaceSingleValue(rhsValue));
}

/** Subtracts two workspaces
 *  @param lhs :: left hand side workspace shared pointer
 *  @param rhs :: right hand side workspace shared pointer
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator-(const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs)
{
  return executeBinaryOperation("Minus",lhs,rhs);
}

/** Subtracts  a single value from a workspace
 *  @param lhs ::      left hand side workspace shared pointer
 *  @param rhsValue :: the single value
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator-(const MatrixWorkspace_sptr lhs, const double& rhsValue)
{
  return executeBinaryOperation("Minus",lhs,createWorkspaceSingleValue(rhsValue));
}

/** Subtracts a workspace from a single value
 *  @param lhsValue :: the single value
 *  @param rhs :: right-hand side workspace shared pointer
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator-(const double& lhsValue, const MatrixWorkspace_sptr rhs)
{
  return executeBinaryOperation("Minus",createWorkspaceSingleValue(lhsValue),rhs);
}
/** Multiply two workspaces
 *  @param lhs :: left hand side workspace shared pointer
 *  @param rhs :: right hand side workspace shared pointer
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator*(const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs)
{
  return executeBinaryOperation("Multiply",lhs,rhs);
}

/** Multiply a workspace and a single value
 *  @param lhs ::      left hand side workspace shared pointer
 *  @param rhsValue :: the single value
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator*(const MatrixWorkspace_sptr lhs, const double& rhsValue)
{
  return executeBinaryOperation("Multiply",lhs,createWorkspaceSingleValue(rhsValue));
}

/** Multiply a workspace and a single value. Allows you to write, e.g., 2*workspace.
 *  @param lhsValue :: the single value
 *  @param rhs ::      workspace shared pointer
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator*(const double& lhsValue, const MatrixWorkspace_sptr rhs)
{
  return executeBinaryOperation("Multiply",createWorkspaceSingleValue(lhsValue),rhs);
}

/** Divide two workspaces
 *  @param lhs :: left hand side workspace shared pointer
 *  @param rhs :: right hand side workspace shared pointer
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator/(const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs)
{
  return executeBinaryOperation("Divide",lhs,rhs);
}

/** Divide a workspace by a single value
 *  @param lhs ::      left hand side workspace shared pointer
 *  @param rhsValue :: the single value
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator/(const MatrixWorkspace_sptr lhs, const double& rhsValue)
{
  return executeBinaryOperation("Divide",lhs,createWorkspaceSingleValue(rhsValue));
}

/** Divide a single value and a workspace. Allows you to write, e.g., 2/workspace.
 *  @param lhsValue :: the single value
 *  @param rhs ::      workspace shared pointer
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator/(const double& lhsValue, const MatrixWorkspace_sptr rhs)
{
  return executeBinaryOperation("Divide",createWorkspaceSingleValue(lhsValue),rhs);
}

/** Adds two workspaces
 *  @param lhs :: left hand side workspace shared pointer
 *  @param rhs :: right hand side workspace shared pointer
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator+=(const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs)
{
  return executeBinaryOperation("Plus",lhs,rhs,true);
}

/** Adds a single value to a workspace
 *  @param lhs ::      workspace shared pointer
 *  @param rhsValue :: the single value
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator+=(const MatrixWorkspace_sptr lhs, const double& rhsValue)
{
  return executeBinaryOperation("Plus",lhs,createWorkspaceSingleValue(rhsValue),true);
}

/** Subtracts two workspaces
 *  @param lhs :: left hand side workspace shared pointer
 *  @param rhs :: right hand side workspace shared pointer
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator-=(const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs)
{
  return executeBinaryOperation("Minus",lhs,rhs,true);
}

/** Subtracts a single value from a workspace
 *  @param lhs ::      workspace shared pointer
 *  @param rhsValue :: the single value
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator-=(const MatrixWorkspace_sptr lhs, const double& rhsValue)
{
  return executeBinaryOperation("Minus",lhs,createWorkspaceSingleValue(rhsValue),true);
}

/** Multiply two workspaces
 *  @param lhs :: left hand side workspace shared pointer
 *  @param rhs :: right hand side workspace shared pointer
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator*=(const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs)
{
  return executeBinaryOperation("Multiply",lhs,rhs,true);
}

/** Multiplies a workspace by a single value
 *  @param lhs ::      workspace shared pointer
 *  @param rhsValue :: the single value
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator*=(const MatrixWorkspace_sptr lhs, const double& rhsValue)
{
  return executeBinaryOperation("Multiply",lhs,createWorkspaceSingleValue(rhsValue),true);
}

/** Divide two workspaces
 *  @param lhs :: left hand side workspace shared pointer
 *  @param rhs :: right hand side workspace shared pointer
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator/=(const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs)
{
  return executeBinaryOperation("Divide",lhs,rhs,true);
}

/** Divides a workspace by a single value
 *  @param lhs ::      workspace shared pointer
 *  @param rhsValue :: the single value
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator/=(const MatrixWorkspace_sptr lhs, const double& rhsValue)
{
  return executeBinaryOperation("Divide",lhs,createWorkspaceSingleValue(rhsValue),true);
}


//----------------------------------------------------------------------
// Now the WorkspaceHelpers methods
//----------------------------------------------------------------------

/** Checks whether a workspace has common bins (or values) in X
 *  @param WS :: The workspace to check
 *  @return True if the bins match
 */
bool WorkspaceHelpers::commonBoundaries(const MatrixWorkspace_const_sptr WS)
{
  if ( !WS->blocksize() || WS->getNumberHistograms() < 2) return true;
  // Quickest check is to see if they are actually all the same vector
  if ( sharedXData(WS) ) return true;

  // But even if they're not they could still match...
  const double commonSum = std::accumulate(WS->readX(0).begin(),WS->readX(0).end(),0.);
  // If this results in infinity or NaN, then we can't tell - return false
  if ( commonSum == std::numeric_limits<double>::infinity() || commonSum != commonSum ) return false;
  const int numHist = WS->getNumberHistograms();
  for (int j = 1; j < numHist; ++j)
  {
    const double sum = std::accumulate(WS->readX(j).begin(),WS->readX(j).end(),0.);
    // If this results in infinity or NaN, then we can't tell - return false
    if ( sum == std::numeric_limits<double>::infinity() || sum != sum ) return false;

    if ( std::abs(commonSum-sum)/std::max<double>(commonSum,sum) > 1.0E-7 ) return false;
  }
  return true;
}

/** Checks whether the bins (X values) of two workspace are the same
 *  @param ws1 :: The first workspace
 *  @param ws2 :: The second workspace
 *  @param firstOnly :: If true, only the first spectrum is checked. If false (the default), all spectra
 *                   are checked and the two workspaces must have the same number of spectra
 *  @return True if the test passes
 */
bool WorkspaceHelpers::matchingBins(const MatrixWorkspace_const_sptr ws1,
                                          const MatrixWorkspace_const_sptr ws2, const bool firstOnly)
{
  // First of all, the first vector must be the same size
  if ( ws1->readX(0).size() != ws2->readX(0).size() ) return false;

  // Now check the first spectrum
  const double firstWS = std::accumulate(ws1->readX(0).begin(),ws1->readX(0).end(),0.);
  const double secondWS = std::accumulate(ws2->readX(0).begin(),ws2->readX(0).end(),0.);
  if ( std::abs(firstWS-secondWS)/std::max<double>(firstWS,secondWS) > 1.0E-7 ) return false;

  // If we were only asked to check the first spectrum, return now
  if (firstOnly) return true;

  // Check that total size of workspace is the same
  if ( ws1->size() != ws2->size() ) return false;
  // If that passes then check whether all the X vectors are shared
  if ( sharedXData(ws1) && sharedXData(ws2) ) return true;

  // If that didn't pass then explicitly check 1 in 10 of the vectors (min 10, max 100)
  const int numHist = ws1->getNumberHistograms();
  int numberToCheck = numHist / 10;
  if (numberToCheck<10) numberToCheck = 10;
  if (numberToCheck>100) numberToCheck = 100;
  int step = numHist / numberToCheck;
  if (!step) step=1;
  for (int i = step; i < numHist; i+=step)
  {
    const double firstWS = std::accumulate(ws1->readX(i).begin(),ws1->readX(i).end(),0.);
    const double secondWS = std::accumulate(ws2->readX(i).begin(),ws2->readX(i).end(),0.);
    if ( std::abs(firstWS-secondWS)/std::max<double>(firstWS,secondWS) > 1.0E-7 ) return false;
  }

  return true;
}

/// Checks whether all the X vectors in a workspace are the same one underneath
bool WorkspaceHelpers::sharedXData(const MatrixWorkspace_const_sptr WS)
{
  const double& first = WS->readX(0)[0];
  const int numHist = WS->getNumberHistograms();
  for (int i = 1; i < numHist; ++i)
  {
    if ( &first != &(WS->readX(i)[0]) ) return false;
  }
  return true;
}

/** Divides the data in a workspace by the bin width to make it a distribution.
 *  Can also reverse this operation (i.e. multiply by the bin width).
 *  Sets the isDistribution() flag accordingly.
 *  @param workspace :: The workspace on which to carry out the operation
 *  @param forwards :: If true (the default) divides by bin width, if false multiplies
 */
void WorkspaceHelpers::makeDistribution(MatrixWorkspace_sptr workspace, const bool forwards)
{
  // Check workspace isn't already in the correct state - do nothing if it is
  if ( workspace->isDistribution() == forwards ) return;

  const int numberOfSpectra = workspace->getNumberHistograms();

  std::vector<double> widths(workspace->readX(0).size());

  for (int i = 0; i < numberOfSpectra; ++i)
  {
	  const MantidVec& X=workspace->readX(i);
	  MantidVec& Y=workspace->dataY(i);
	  MantidVec& E=workspace->dataE(i);
	  std::adjacent_difference(X.begin(),X.end(),widths.begin()); // Calculate bin widths

    // RJT: I'll leave this in, but X should never be out of order. 
    // If it is there'll be problems elsewhere...
	  if (X.front()>X.back()) // If not ascending order
		  std::transform(widths.begin(),widths.end(),widths.begin(),std::negate<double>());

	  if (forwards)
	  {
		  std::transform(Y.begin(),Y.end(),widths.begin()+1,Y.begin(),std::divides<double>());
		  std::transform(E.begin(),E.end(),widths.begin()+1,E.begin(),std::divides<double>());
	  }
	  else
	  {
		  std::transform(Y.begin(),Y.end(),widths.begin()+1,Y.begin(),std::multiplies<double>());
		  std::transform(E.begin(),E.end(),widths.begin()+1,E.begin(),std::multiplies<double>());
	  }
  }
  workspace->isDistribution(forwards);
}





} // namespace API
} // namespace Mantid
