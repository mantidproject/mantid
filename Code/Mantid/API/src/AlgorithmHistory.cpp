//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AlgorithmHistory.h"
#include "boost/date_time/posix_time/posix_time.hpp"

namespace Mantid
{
namespace API
{

/// Constructor
  AlgorithmHistory::AlgorithmHistory(const std::string& name, const std::string& version, 
    const dateAndTime& start, const double& duration,
    const std::vector<AlgorithmParameter>& parameters):
    m_name(name),
    m_version(version),
    m_executionDate(start),
    m_executionDuration(duration),
    m_parameters(parameters)
{  
}
  
/// Default Constructor
AlgorithmHistory::AlgorithmHistory()
// strings have their own default constructor
:
m_executionDuration(0.0)
{
}

/// Destructor
AlgorithmHistory::~AlgorithmHistory()
{
}
/*!
  Standard Copy Constructor
  \param A :: AlgorithmHistory Item to copy
*/

AlgorithmHistory::AlgorithmHistory(const AlgorithmHistory& A)
:
m_name(A.m_name),m_version(A.m_version),m_executionDate(A.m_executionDate),
m_executionDuration(A.m_executionDuration),m_parameters(A.m_parameters)
{
}
/*!
  Standard Assignment operator
  \param A :: AlgorithmHistory Item to assign to 'this'
*/
AlgorithmHistory& AlgorithmHistory::operator=(const AlgorithmHistory& A)
  {
    if (this!=&A)
    {
      m_name=A.m_name;
      m_version=A.m_version;
      m_executionDate=A.m_executionDate;
      m_executionDuration=A.m_executionDuration;
      m_parameters=A.m_parameters;
    }
    return *this;
  }

} // namespace API
} // namespace Mantid

