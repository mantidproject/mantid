#include "MantidKernel/System.h"
#include "MantidAPI/Workspace.h"
#include "boost/shared_ptr.hpp"

namespace Mantid
{

namespace Kernel
{
  class Logger;
}
	
  namespace API
  {
    // Workspace operator overloads
    Workspace_sptr DLLExport operator+(const Workspace_sptr lhs, const Workspace_sptr rhs);
    Workspace_sptr DLLExport operator-(const Workspace_sptr lhs, const Workspace_sptr rhs);
    Workspace_sptr DLLExport operator*(const Workspace_sptr lhs, const Workspace_sptr rhs);
    Workspace_sptr DLLExport operator/(const Workspace_sptr lhs, const Workspace_sptr rhs);

    Workspace_sptr DLLExport operator+(const Workspace_sptr lhs, const double rhsValue);
    Workspace_sptr DLLExport operator-(const Workspace_sptr lhs, const double rhsValue);
    Workspace_sptr DLLExport operator*(const Workspace_sptr lhs, const double rhsValue);
    Workspace_sptr DLLExport operator/(const Workspace_sptr lhs, const double rhsValue);

    Workspace_sptr executeBinaryOperation(std::string AlgorithmName, 
      const Workspace_sptr lhs, const Workspace_sptr rhs);

    Workspace_sptr createWorkspaceSingleValue(const double rhsValue);
  }
}
