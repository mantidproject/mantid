//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/WorkspaceHistory.h"

namespace Mantid
{
namespace API
{
///Default Constructor
  WorkspaceHistory::WorkspaceHistory()
{}

/// Constructor
WorkspaceHistory::WorkspaceHistory(const EnvironmentHistory& environment, 
                                   const std::vector<AlgorithmHistory>& algorithms)
:
m_environment(environment),m_algorithms(algorithms)
{  
}

/// Destructor
WorkspaceHistory::~WorkspaceHistory()
{
}

/*!
  Standard Copy Constructor
  \param A :: WorkspaceHistory Item to copy
*/
WorkspaceHistory::WorkspaceHistory(const WorkspaceHistory& A)
:
  m_environment(A.m_environment),m_algorithms(A.m_algorithms)
{}

  /*!
  Standard Assignment operator
  \param A :: WorkspaceHistory Item to assign to 'this'
*/
WorkspaceHistory& WorkspaceHistory::operator=(const WorkspaceHistory& A)
{
  if (this!=&A)
  {
    m_environment=A.m_environment;
    m_algorithms=A.m_algorithms;
  }
  return *this;
}


} // namespace API
} // namespace Mantid

