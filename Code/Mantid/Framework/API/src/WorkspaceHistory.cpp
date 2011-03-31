//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/WorkspaceHistory.h"

namespace Mantid
{
namespace API
{

///Default Constructor
WorkspaceHistory::WorkspaceHistory() : m_environment(), m_algorithms()
{}

/// Destructor
WorkspaceHistory::~WorkspaceHistory()
{}

/**
  Standard Copy Constructor
  @param A :: WorkspaceHistory Item to copy
 */
WorkspaceHistory::WorkspaceHistory(const WorkspaceHistory& A) :
  m_environment(A.m_environment),m_algorithms(A.m_algorithms)
{}

/// Returns a const reference to the algorithmHistory
const std::vector<AlgorithmHistory>& WorkspaceHistory::getAlgorithmHistories() const
{
  return m_algorithms;
}
/// Returns a const reference to the EnvironmentHistory
  const Kernel::EnvironmentHistory& WorkspaceHistory::getEnvironmentHistory() const
  {
	  return m_environment;
  }

/// Copy the algorithm history from another WorkspaceHistory into this one
void WorkspaceHistory::copyAlgorithmHistory(const WorkspaceHistory& otherHistory)
{
  // Don't copy one's own history onto oneself
  if (this != &otherHistory)
  {
    m_algorithms.insert(
        m_algorithms.end(),
        otherHistory.getAlgorithmHistories().begin(),
        otherHistory.getAlgorithmHistories().end()    );
  }
}

/// Append an AlgorithmHistory to this WorkspaceHistory
void WorkspaceHistory::addAlgorithmHistory(const AlgorithmHistory& algHistory)
{
  m_algorithms.push_back(algHistory);
}

/*
 Return the history length
 */
size_t WorkspaceHistory::length() const
{
  return m_algorithms.size();
}

/** Prints a text representation of itself
 *  @param os :: The ouput stream to write to
 *  @param indent :: an indentation value to make pretty printing of object and sub-objects
 */
void WorkspaceHistory::printSelf(std::ostream& os, const int indent)const
{

  os << std::string(indent,' ')  << m_environment << std::endl;

  std::vector<AlgorithmHistory>::const_iterator it;
  os << std::string(indent,' ') << "Histories:" <<std::endl;

  for (it=m_algorithms.begin();it!=m_algorithms.end();it++)
  {
    os << std::endl;
    it->printSelf( os, indent+2 );
  }
}

/** Prints a text representation
 * @param os :: The ouput stream to write to
 * @param WH :: The WorkspaceHistory to output
 * @returns The ouput stream
 */
std::ostream& operator<<(std::ostream& os, const WorkspaceHistory& WH)
{
  WH.printSelf(os);
  return os;
}

} // namespace API
} // namespace Mantid
