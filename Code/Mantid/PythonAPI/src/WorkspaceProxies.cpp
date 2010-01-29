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
// Binary operation for two workspaces
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


// Binary operation for workspace and double
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

/// Binary operation on double and workspace
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
