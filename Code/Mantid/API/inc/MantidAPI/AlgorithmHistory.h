#ifndef MANTID_API_ALGORITHMHISTORY_H_
#define MANTID_API_ALGORITHMHISTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/PropertyHistory.h"
#include <ctime>
#include <vector>

namespace Mantid
{
namespace API
{
class Algorithm;
/** @class AlgorithmHistory AlgorithmHistory.h API/MAntidAPI/AlgorithmHistory.h

    This class stores information about the Command History used by algorithms on a workspace.

    @author Dickon Champion, ISIS, RAL
    @date 21/01/2008

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
class DLLExport AlgorithmHistory
{
public:
  /// The date-and-time will be stored as the ctime time_t type
  typedef time_t dateAndTime;

  explicit AlgorithmHistory(const Algorithm* const alg, const dateAndTime& start = 0, const double& duration = -1);
  virtual ~AlgorithmHistory();
  AlgorithmHistory& operator=(const AlgorithmHistory&);
  AlgorithmHistory(const AlgorithmHistory&);

  void addExecutionInfo(const dateAndTime& start, const double& duration);

  // get functions
  /// get name of algorithm in history const
  const std::string& name() const {return m_name;}
  /// get version number of algorithm in history const
  const int& version() const {return m_version;}
  // get execution duration in hist const
  double executionDuration() const { return m_executionDuration;}
  // get execution date in hist const
  dateAndTime executionDate() const {return m_executionDate;}

  /// get parameter list of algorithm in history const
  const std::vector<Kernel::PropertyHistory>& getProperties() const {return m_properties;}
  /// print contents of object
  void printSelf(std::ostream&,const int indent = 0) const;

  ///this is required for boost.python
  bool operator==(const AlgorithmHistory &other) const
  {
    if (name() == other.name() && version() == other.version() && getProperties() == other.getProperties())
    {
      return true;
    }

    return false;
  }

private:
  /// The name of the Algorithm
  std::string m_name;
  /// The version of the algorithm
  int m_version;
  /// The execution date of the algorithm
  dateAndTime m_executionDate;
  /// The execution duration of the algorithm
  double m_executionDuration;
  /// The PropertyHistory's defined for the algorithm
  std::vector<Kernel::PropertyHistory> m_properties;
};

DLLExport std::ostream& operator<<(std::ostream&, const AlgorithmHistory&);

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_ALGORITHMHISTORY_H_*/
