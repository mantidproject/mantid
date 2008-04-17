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
    AlgorithmHistory::AlgorithmHistory(const std::string& name, const int& version, 
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
    /** Prints a text representation of itself
    * @param os The ouput stream to write to
    */
    void AlgorithmHistory::printSelf(std::ostream& os, const int indent)const
    {
      os << std::string(indent,' ') << "Name : " << m_name << std::endl;
      os << std::string(indent,' ') << "Version: " << m_version << std::endl;
      char buffer [25];
      strftime (buffer,25,"%Y-%b-%d %H:%M:%S",localtime(&m_executionDate));
      os << std::string(indent,' ') << "Execution Date: " << buffer<<std::endl;
      os << std::string(indent,' ') << "Execution Duration: "<< m_executionDuration << " seconds" << std::endl; 
      std::vector<AlgorithmParameter>::const_iterator it;
      os << std::string(indent,' ') << "Parameters:" <<std::endl;

      for (it=m_parameters.begin();it!=m_parameters.end();it++)
      {
        os << std::endl;
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
        m_parameters=A.m_parameters;
      }
      return *this;
    }
    /** Prints a text representation
    * @param os The ouput stream to write to
    * @param AP The AlgorithmHistory to output
    * @returns The ouput stream
    */
    std::ostream& operator<<(std::ostream& os, const AlgorithmHistory& AH)
    {
      AH.printSelf(os);
      return os;
    }

  } // namespace API
} // namespace Mantid

