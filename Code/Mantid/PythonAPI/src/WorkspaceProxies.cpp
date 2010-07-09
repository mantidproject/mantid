//---------------------------------------
// Includes
//---------------------------------------
#include <MantidPythonAPI/WorkspaceProxies.h>
#include <MantidAPI/WorkspaceOpOverloads.h>

using namespace Mantid::PythonAPI;

//****************************************
//
// WorkspaceAlgebraProxy
//
//****************************************

/// Binary operation for two workspaces
/// @param lhs the left hand side workspace of the operation
/// @param rhs the right hand side workspace of the operation
/// @param op The operation
/// @param inplace is this is an inplace operation (i.e. does the output overwrite the lhs
/// @returns The resulting workspace
WorkspaceAlgebraProxy::wraptype_ptr WorkspaceAlgebraProxy::performBinaryOp(const wraptype_ptr lhs, const wraptype_ptr rhs, char op, bool inplace)
{
  wraptype_ptr result;
  std::string name(lhs->getName());
  if( inplace )
  {
    switch( op )
    {
    case 'p':
      lhs += rhs;
      break;
    case 'm':
      lhs -= rhs;
      break;
    case 't':
      lhs *= rhs;
      break;
    case 'd':
          lhs /= rhs;
    }
    result = lhs;
  }
  else
  {
    switch( op )
    {
    case 'p':
      result = lhs + rhs;
      break;
    case 'm':
      result = lhs - rhs;
      break;
    case 't':
      result = lhs * rhs;
      break;
    case 'd':
      result = lhs / rhs;
    }
    name += std::string("_") + op + std::string("_") + rhs->getName();
  }
  Mantid::API::AnalysisDataService::Instance().addOrReplace(name, result);
  return result;
}


/** 
* Perform the given binary operation on a workspace and a double
* @param lhs The left-hand side of the operation
* @param rhs The right-hand side of the operation
* @param op One of 'p', 'm', 't', 'd' to denote the required operation
* @param inplace If true, then the lhs argument is replaced by the result of the operation.
*/
WorkspaceAlgebraProxy::wraptype_ptr WorkspaceAlgebraProxy::performBinaryOp(const wraptype_ptr lhs, double rhs, char op, bool inplace)
{
  wraptype_ptr result;
  std::string name(lhs->getName());
  if( inplace )
  {
    switch( op )
    {
    case 'p':
      lhs += rhs;
      break;
    case 'm':
      lhs -= rhs;
      break;
    case 't':
      lhs *= rhs;
      break;
    case 'd':
      lhs /= rhs;
      break;
    }
    result = lhs;
  }
  else
  {
    switch( op )
    {
    case 'p':
      result = lhs + rhs;
      break;
    case 'm':
      result = lhs - rhs;
      break;
    case 't':
      result = lhs * rhs;
      break;
    case 'd':
      result = lhs / rhs;
    }
    std::ostringstream os;
    os << rhs;
    name += std::string("_") + op + std::string("_") + os.str();
  }
  Mantid::API::AnalysisDataService::Instance().addOrReplace(name, result);
  return result;
}

/** Binary operation on double and workspace
 @param lhs The left-hand side of the operation
 @param rhs The right-hand side of the operation
 @param op One of 'p', 'm', 't', 'd' to denote the required operation
 @returns The resulting workspace
 */
WorkspaceAlgebraProxy::wraptype_ptr WorkspaceAlgebraProxy::performBinaryOp(double lhs, const wraptype_ptr rhs, char op)
{
  wraptype_ptr result;
  std::ostringstream os;
  os << lhs;
  std::string name(os.str());
  switch( op )
  {
  case 'p':
    result = rhs + lhs;
    break;
  case 'm':
    result = lhs - rhs;
    break;
  case 't':
    result = lhs * rhs;
    break;
  case 'd':
    result = lhs / rhs;
  }
  name += std::string("_") + op + std::string("_") + rhs->getName();
  Mantid::API::AnalysisDataService::Instance().addOrReplace(name, result);
  return result;
}
