//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace API
{

using Kernel::Property;
using Kernel::dateAndTime;

/** Constructor
 *  @param alg      A pointer to the algorithm for which the history should be constructed
 *  @param start    The start time of the algorithm execution (optional)
 *  @param duration The time (in seconds) that it took to run this algorithm (optional)
 *  @param uexeccount an  unsigned int for algorithm execution order
 */
AlgorithmHistory::AlgorithmHistory(const Algorithm* const alg, const dateAndTime& start, const double& duration,unsigned int uexeccount) :
  m_name(alg->name()), m_version(alg->version()), m_executionDate(start), m_executionDuration(duration),m_execCount(uexeccount)
{
  // Now go through the algorithm's properties and create the PropertyHistory objects.
  const std::vector<Property*>& properties = alg->getProperties();
  std::vector<Property*>::const_iterator it;
  for (it = properties.begin(); it != properties.end(); ++it)
  {
    m_properties.push_back( (*it)->createHistory() );
  }
}

/// Destructor
AlgorithmHistory::~AlgorithmHistory()
{}

/*!
    Construct AlgorithmHistory by name. Can be used for rstoring the history from saved records.
    \param name The algorithm name.
    \param vers The algorithm version.
    \param start The start time of the algorithm execution (optional).
    \param duration The time (in seconds) that it took to run this algorithm (optional).
	 \param uexeccount  an  unsigned int for algorithm execution order
 */
AlgorithmHistory::AlgorithmHistory(const std::string& name, int vers, const dateAndTime& start, const double& duration,unsigned int uexeccount) :
  m_name(name),m_version(vers),m_executionDate(start),
  m_executionDuration(duration),m_execCount(uexeccount)
{
}

/*!
    Standard Copy Constructor
    \param A :: AlgorithmHistory Item to copy
 */
AlgorithmHistory::AlgorithmHistory(const AlgorithmHistory& A) :
  m_name(A.m_name),m_version(A.m_version),m_executionDate(A.m_executionDate),
  m_executionDuration(A.m_executionDuration),m_properties(A.m_properties),m_execCount(A.m_execCount)
{
}

/** Add details of an algorithm's execution to an existing history object
 *  @param start    The start time of the algorithm execution
 *  @param duration The time (in seconds) that it took to run this algorithm
 */
void AlgorithmHistory::addExecutionInfo(const dateAndTime& start, const double& duration)
{
  m_executionDate = start;
  m_executionDuration = duration;
}

/** Add a property to the history.
    @param name The name of the property
    @param value The value of the property
    @param isdefault True if the property is default
    @param direction The direction of the property
 */
  void AlgorithmHistory::addProperty(const std::string& name,const std::string& value, bool isdefault, 
				     const unsigned int& direction)
{
  m_properties.push_back(Kernel::PropertyHistory(name,value,"",isdefault, direction));
}

/** Prints a text representation of itself
 *  @param os The ouput stream to write to
 *  @param indent an indentation value to make pretty printing of object and sub-objects
 */
void AlgorithmHistory::printSelf(std::ostream& os, const int indent)const
{
  os << std::string(indent,' ') << "Algorithm: " << m_name;
  os << std::string(indent,' ') << " v" << m_version << std::endl;
  if (m_executionDate != Mantid::Kernel::DateAndTime::defaultTime)
  {
    char buffer [25];
    std::tm date_as_tm = Mantid::Kernel::DateAndTime::to_localtime_tm( m_executionDate );
    strftime (buffer,25,"%Y-%b-%d %H:%M:%S", &date_as_tm);
    os << std::string(indent,' ') << "Execution Date: " << buffer<<std::endl;
    os << std::string(indent,' ') << "Execution Duration: "<< m_executionDuration << " seconds" << std::endl;
  }
  std::vector<Kernel::PropertyHistory>::const_iterator it;
  os << std::string(indent,' ') << "Parameters:" <<std::endl;

  for (it=m_properties.begin();it!=m_properties.end();it++)
  {
    it->printSelf( os, indent+2 );
  }
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
    m_properties=A.m_properties;
  }
  return *this;
}

/** Prints a text representation
 * @param os The ouput stream to write to
 * @param AH The AlgorithmHistory to output
 * @returns The ouput stream
 */
std::ostream& operator<<(std::ostream& os, const AlgorithmHistory& AH)
{
  AH.printSelf(os);
  return os;
}

} // namespace API
} // namespace Mantid
