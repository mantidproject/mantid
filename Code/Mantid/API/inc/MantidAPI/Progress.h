#ifndef MANTID_API_PROGRESS_H_
#define MANTID_API_PROGRESS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
//#include "MantidAPI/IAlgorithm.h"
//#include "MantidAPI/AlgorithmManager.h"
//#include "MantidAPI/WorkspaceFactory.h"
//#include "MantidAPI/WorkspaceOpOverloads.h"
//#include "MantidKernel/PropertyManager.h"
//#include "MantidKernel/Property.h"
//#include "MantidAPI/WorkspaceProperty.h"
//#include "MantidKernel/Logger.h"
//#include "MantidKernel/Exception.h"

//#include <boost/shared_ptr.hpp>
//#include <Poco/ActiveMethod.h>
//#include <Poco/NotificationCenter.h>
//#include <Poco/Notification.h>
//#include <Poco/NObserver.h>
//#include <string>
//#include <vector>
//#include <map>

namespace Mantid
{
namespace API
{
class Algorithm;

/** 
 Helper class for reporting progress from algorithms.

 @author Roman Tolchenov, Tessella Support Services plc
 @date 06/02/2009

 Copyright &copy; 2009 STFC Rutherford Appleton Laboratory

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
class DLLExport Progress
{
public:
  /** Creates a Progress instance
      @param alg   Algorithm reporting its progress
      @param start Starting progress
      @param end   Ending progress
      @param n     Number of times report(...) method will be called. 
      @param step  The frequency in % with which report(...) actually sends the notification.
  */
  Progress(Algorithm* alg,double start,double end, int n, int step=1)
    :m_alg(alg),m_start(start),m_end(end),m_ifirst(0),m_n(n),m_step(n*step/100/static_cast<int>(end-start)),
    m_dp((end-start)/(n-1)),m_i(0)
  {if (m_step <= 0) m_step = 1;}
  
  /** Creates a Progress instance
      @param alg    Algorithm reporting its progress
      @param start  Starting progress
      @param end    Ending progress
      @param ifirst Initial value of the loop counter
      @param n      Upper bound for the loop counter
      @param step   The frequency in % with which report(...) actually sends the notification.
  */
  Progress(Algorithm* alg,double start,double end, int ifirst, int n, int step)
    :m_alg(alg),m_start(start),m_end(end),m_ifirst(ifirst),m_n(n),m_step((n-ifirst)*step/100/static_cast<int>(end-start)),
    m_dp((end-start)/(n-1)),m_i(ifirst)
  {if (m_step <= 0) m_step = 1;}
  
  void report(const std::string& msg = "");
  void report(int i,const std::string& msg = "");
  
private:
  Algorithm* const m_alg;  ///< Owning algorithm
  const double m_start;    ///< Starting progress
  const double m_end;      ///< Ending progress
  const int m_ifirst;      ///< Loop counter initial value
  const int m_n;           ///< Loop counter upper bound
  int m_step;        ///< Frequency of sending the notification (every m_step times)
  const double m_dp;       ///< Progress increment at each loop
  int m_i;           ///< Loop counter
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_PROGRESS_H_*/
