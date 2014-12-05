#ifndef MANTID_API_ALGORITHMHISTORY_H_
#define MANTID_API_ALGORITHMHISTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/PropertyHistory.h"
#include "MantidKernel/DateAndTime.h"
#include <nexus/NeXusFile.hpp>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <ctime>
#include <vector>
#include <set>

namespace Mantid
{
namespace API
{
  class IAlgorithm;
  class Algorithm;
  class AlgorithmHistory;

  //typedefs for algorithm history pointers
  typedef boost::shared_ptr<AlgorithmHistory> AlgorithmHistory_sptr;
  typedef boost::shared_ptr<const AlgorithmHistory> AlgorithmHistory_const_sptr;

  typedef std::set<AlgorithmHistory_sptr, 
    boost::function<bool(const AlgorithmHistory_const_sptr, const AlgorithmHistory_const_sptr)> > AlgorithmHistories;

/** @class AlgorithmHistory AlgorithmHistory.h API/MAntidAPI/AlgorithmHistory.h

    This class stores information about the Command History used by algorithms on a workspace.

    @author Dickon Champion, ISIS, RAL
    @date 21/01/2008

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */


class MANTID_API_DLL AlgorithmHistory
{
    
public:
  /// History container
  
  /// The date-and-time will be stored as the Mantid::Kernel::DateAndTime type
  explicit AlgorithmHistory(const Algorithm* const alg, 
                            const Kernel::DateAndTime& start = Kernel::DateAndTime::defaultTime(),
                            const double& duration = -1.0,std::size_t uexeccount = 0);
  virtual ~AlgorithmHistory();
  AlgorithmHistory& operator=(const AlgorithmHistory&);
  AlgorithmHistory(const AlgorithmHistory&);
  AlgorithmHistory(const std::string& name, int vers, const Kernel::DateAndTime& start = Kernel::DateAndTime::defaultTime(),
                   const double& duration = -1.0, std::size_t uexeccount = 0);
  void addExecutionInfo(const Kernel::DateAndTime& start, const double& duration);
  void addProperty(const std::string& name,const std::string& value,bool isdefault, 
                   const unsigned int& direction = 99);

  /// add a child algorithm history record to this history object
  void addChildHistory(AlgorithmHistory_sptr childHist);
  // get functions
  /// get name of algorithm in history const
  const std::string& name() const {return m_name;}
  /// get version number of algorithm in history const
  const int& version() const {return m_version;}
  /// get execution duration
  double executionDuration() const { return m_executionDuration;}
  /// get execution date 
  Mantid::Kernel::DateAndTime executionDate() const {return m_executionDate;}
  ///get the execution count
  const std::size_t& execCount() const {return m_execCount;}
  /// get parameter list of algorithm in history const
  const Mantid::Kernel::PropertyHistories& getProperties() const {return m_properties;}
  /// get the child histories of this history object
  const AlgorithmHistories& getChildHistories() const { return m_childHistories; }
  /// Retrieve a child algorithm history by index
  AlgorithmHistory_sptr getChildAlgorithmHistory(const size_t index) const;
    /// Add operator[] access
  AlgorithmHistory_sptr operator[](const size_t index) const;
  /// Retrieve the number of child algorithms
  size_t childHistorySize() const;
  /// print contents of object
  void printSelf(std::ostream&,const int indent = 0) const;
  /// Less than operator
  inline bool operator<(const AlgorithmHistory &other) const
  {
    return (execCount() < other.execCount()) ;
  }
  /// Equality operator
  inline bool operator==(const AlgorithmHistory &other) const
  {
    return (execCount() == other.execCount() &&
            name() == other.name());
  }
  ///Less than operator for pointers
  inline bool compareHistory(const boost::shared_ptr<AlgorithmHistory> lhs, 
    const boost::shared_ptr<AlgorithmHistory> rhs)
  {
    return *lhs < *rhs;
  }
  /// Create a concrete algorithm based on a history record
  boost::shared_ptr<IAlgorithm> createAlgorithm() const;
  /// Create an child algorithm from a history record at a given index
  boost::shared_ptr<IAlgorithm> getChildAlgorithm(const size_t index) const;
  /// Write this history object to a nexus file
  void saveNexus(::NeXus::File* file, int& algCount) const;
  // Set the execution count 
  void setExecCount(std::size_t execCount) { m_execCount = execCount; }
  /// Set data on history after it is created
  void fillAlgorithmHistory(const Algorithm* const alg, 
                       const Kernel::DateAndTime& start = Kernel::DateAndTime::defaultTime(),
                       const double& duration = -1.0,std::size_t uexeccount = 0);
  // Allow Algorithm::execute to change the exec count & duration after the algorithm was executed
  friend class Algorithm;
  
private:
  //private constructor
  AlgorithmHistory();
  // Set properties of algorithm
  void setProperties(const Algorithm* const alg);
  /// The name of the Algorithm
  std::string m_name;
  /// The version of the algorithm
  int m_version;
  /// The execution date of the algorithm
  Mantid::Kernel::DateAndTime m_executionDate;
  /// The execution duration of the algorithm
  double m_executionDuration;
  /// The PropertyHistory's defined for the algorithm
  Mantid::Kernel::PropertyHistories m_properties;
  ///count keeps track of execution order of an algorithm
  std::size_t m_execCount;
  /// set of child algorithm histories for this history record
  AlgorithmHistories m_childHistories;
};

struct CompareHistory
{
    ///Less than operator for pointers
  static bool compare(const AlgorithmHistory_const_sptr lhs, 
    const AlgorithmHistory_const_sptr rhs)
  {
    return (*lhs) < (*rhs);
  }
};

MANTID_API_DLL std::ostream& operator<<(std::ostream&, const AlgorithmHistory&);

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_ALGORITHMHISTORY_H_*/
