//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/EnvironmentHistory.h"

namespace Mantid
{
  namespace API
  {

    /// Constructor
    EnvironmentHistory::EnvironmentHistory(std::string version, std::string osName, std::string osVersion, std::string userName)
      :
    m_version(version),m_osName(osName),
      m_osVersion(osVersion),m_userName(userName)
    {
    }

    /// Destructor
    EnvironmentHistory::~EnvironmentHistory()
    {
    }
    /// Default Constructor
    EnvironmentHistory::EnvironmentHistory()
    {
    }
    /*!
    Standard Copy Constructor
    \param A :: AlgorithmParameter Item to copy
    */

    EnvironmentHistory::EnvironmentHistory(const EnvironmentHistory& A)
      :
    m_version(A.m_version),m_osName(A.m_osName),m_osVersion(A.m_osVersion),m_userName(A.m_userName)
    {}

    /*!
    Standard Assignment operator
    \param A :: AlgorithmParameter Item to assign to 'this'
    */
    EnvironmentHistory& EnvironmentHistory::operator=(const EnvironmentHistory& A)
    {
      if (this!=&A)
      {
        m_version=A.m_version;
        m_osName=A.m_osName;
        m_osVersion=A.m_osVersion;
        m_userName=A.m_userName;
      }
      return *this;  
    }
    /** Prints a text representation of itself
    * @param os The ouput stream to write to
    */
    void EnvironmentHistory::printSelf(std::ostream& os, const int indent) const
    {
      os << std::string(indent,' ')  << "Framework Version : " << m_version << std::endl;
      os << std::string(indent,' ') << "OS name: " << m_osName << std::endl;
      os << std::string(indent,' ')  << "OS version: " << m_osVersion << std::endl;
      os << std::string(indent,' ') << "username: "<< m_userName << std::endl;       
    }

    /** Prints a text representation
    * @param os The ouput stream to write to
    * @param EH The EnvironmentHistory to output
    * @returns The ouput stream
    */
    std::ostream& operator<<(std::ostream& os, const EnvironmentHistory& EH)
    {
      EH.printSelf(os);
      return os;
    }


  } // namespace API
} // namespace Mantid
