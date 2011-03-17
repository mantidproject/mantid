#ifndef MANTID_API_PROGRESS_H_
#define MANTID_API_PROGRESS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"

namespace Mantid
{
namespace API
{
class Algorithm;

/** 
 Helper class for reporting progress from algorithms.

 @author Roman Tolchenov, Tessella Support Services plc
 @date 06/02/2009

 Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  Progress();

  Progress(Algorithm* alg,double start,double end, int numSteps);

  virtual void report(const std::string& msg = "");
  virtual void report(int i,const std::string& msg = "");
  virtual void reportIncrement(int inc, const std::string& msg = "");
  virtual void setNumSteps(int nsteps);
  
private:
  /// Owning algorithm
  Algorithm* const m_alg;

protected:
  /// Starting progress
  double m_start;
  /// Ending progress
  double m_end;
  /// Loop counter initial value
  int m_ifirst;
  /// Loop counter upper bound
  int m_numSteps;
  /// Frequency of sending the notification (every m_step times)
  int m_notifyStep;
  /// Progress increment at each loop
  double m_step;
  /// Loop counter
  int m_i;
  /// Last loop counter value the was a peport
  int m_last_reported;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_PROGRESS_H_*/
