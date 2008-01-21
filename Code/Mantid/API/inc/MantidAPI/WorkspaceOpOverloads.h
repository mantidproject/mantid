#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/Logger.h"

#include "boost/shared_ptr.hpp"
namespace Mantid
{
  namespace API
  {
    // Workspace operator overloads
    Workspace_sptr DLLExport operator+(const Workspace_sptr lhs, const Workspace_sptr rhs);
    Workspace_sptr DLLExport operator-(const Workspace_sptr lhs, const Workspace_sptr rhs);
    Workspace_sptr DLLExport operator*(const Workspace_sptr lhs, const Workspace_sptr rhs);
    Workspace_sptr DLLExport operator/(const Workspace_sptr lhs, const Workspace_sptr rhs);

    Workspace_sptr DLLExport executeBinaryOperation(std::string AlgorithmName, 
      const Workspace_sptr lhs, const Workspace_sptr rhs);
  }
}
