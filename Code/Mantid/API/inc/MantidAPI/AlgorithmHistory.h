#ifndef MANTID_DATAOBJECTS_ALGORITHMHISTORY_H_
#define MANTID_DATAOBJECTS_ALGORITHMHISTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AlgorithmParameter.h"
#include "boost/date_time/posix_time/posix_time.hpp"

#include <ctime>
#include <vector>

namespace Mantid
{
namespace API
{

/** @class AlgorithmHistory AlgorithmHistory.h API/MAntidAPI/AlgorithmHistory.h

    This class stores information about the Command History used by algorithms on a workspace.

    @author Dickon Champion, ISIS, RAL
    @date 21/01/2008
    
    Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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
    /// The date-and-time will be stored as the boost ptime type
  typedef boost::posix_time::ptime dateAndTime;
  typedef boost::posix_time::time_duration timeDuration;
  AlgorithmHistory();
  AlgorithmHistory(const std::string&, const std::string&, 
    const dateAndTime&, const timeDuration&,
    const std::vector<AlgorithmParameter>&);
	virtual ~AlgorithmHistory();
  AlgorithmHistory& operator=(const AlgorithmHistory&);
  AlgorithmHistory(const AlgorithmHistory&);
  void addparam(std::string, std::string, std::string, bool, unsigned int);
  void setduration(timeDuration);
private:
  /// The name of the Algorithm
  std::string m_name;
  /// The version of the algorithm
  std::string m_version;
  /// The execution date of the algorithm
  dateAndTime m_executionDate;
    /// The execution duration of the algorithm
  timeDuration m_executionDuration;
  std::vector<AlgorithmParameter> m_parameters;

};

} // namespace API
} // namespace Mantid

#endif /*MANTID_DATAOBJECTS_ALGORITHMHISTORY_H_*/
