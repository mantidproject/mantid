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
    /** Prints a text representation of itself
    * @param os The ouput stream to write to
	* @param indent an indentation value to make pretty printing of object and sub-objects
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
    * @param os The ouput stream to write to
    * @param WH The WorkspaceHistory to output
    * @returns The ouput stream
    */
    std::ostream& operator<<(std::ostream& os, const WorkspaceHistory& WH)
    {
      WH.printSelf(os);
      return os;
    }

  } // namespace API
} // namespace Mantid

