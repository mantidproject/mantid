//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAPI/Algorithm.h"

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

/// Append the algorithm history from another WorkspaceHistory into this one
void WorkspaceHistory::addHistory(const WorkspaceHistory& otherHistory)
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
void WorkspaceHistory::addHistory(const AlgorithmHistory& algHistory)
{
  m_algorithms.push_back(algHistory);
}

/*
 Return the history length
 */
size_t WorkspaceHistory::size() const
{
  return m_algorithms.size();
}

/**
 * Retrieve an algorithm history by index
 * @param index ::  An index within the workspace history
 * @returns A reference to a const AlgorithmHistory object
 */
const AlgorithmHistory & WorkspaceHistory::getAlgorithmHistory(const size_t index) const
{
  if( index > this->size() )
  {
    throw std::out_of_range("WorkspaceHistory::getAlgorithmHistory() - Index out of range");
  }
  return m_algorithms[index];
}

/**
 *  Create an algorithm from a history record at a given index
 * @param index ::  An index within the workspace history
 * @returns A shared pointer to an algorithm object
 */
boost::shared_ptr<IAlgorithm> WorkspaceHistory::getAlgorithm(const size_t index) const
{
  return Algorithm::fromHistory(this->getAlgorithmHistory(index));
}

/**
 * Convenience function for retrieving the last algorithm
 * @returns A shared pointer to the algorithm
 */
boost::shared_ptr<IAlgorithm> WorkspaceHistory::lastAlgorithm() const
{
  if( m_algorithms.empty() )
  {
    throw std::out_of_range("WorkspaceHistory::lastAlgorithm() - History contains no algorithms.");
  }
  return this->getAlgorithm(this->size() - 1);
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
