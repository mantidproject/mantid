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


} // namespace API
} // namespace Mantid
