//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AlgorithmHistory.h"


namespace Mantid
{
namespace API
{

/// Constructor
  AlgorithmHistory::AlgorithmHistory(const std::string& name, const std::string& version, 
    const dateAndTime& start, const timeDuration& duration,
    const std::vector<AlgorithmParameter>& parameters):
    m_name(name),
    m_version(version),
    m_executionDuration(duration),
    m_executionDate(start),
    m_parameters(parameters)
{  

  }
/// Default Constructor
AlgorithmHistory::AlgorithmHistory()
// strings have their own default constructor
//m_executionDate(boost::posix_time::not_a_date_time)
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
m_name(A.m_name),m_version(A.m_version),m_executionDuration(A.m_executionDuration),
m_executionDate(A.m_executionDate),m_parameters(A.m_parameters)
{}

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

  void AlgorithmHistory::addparam(std::string name, std::string value, std::string type,bool isdefault, unsigned int direction)
{
m_parameters.push_back(AlgorithmParameter::AlgorithmParameter(name, value, type,isdefault,direction)); 
}

void AlgorithmHistory::setduration(timeDuration duration)
{
  m_executionDuration=duration;
}


} // namespace API
} // namespace Mantid

