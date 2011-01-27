#ifndef MANTID_KERNEL_TIMER_H_
#define MANTID_KERNEL_TIMER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include <boost/timer.hpp>
#include <ostream>
#include <string>

namespace Mantid
{
namespace Kernel
{
/** A simple class that provides a wall-clock (not processor time) timer.

    @author Russell Taylor, Tessella plc
    @date 29/04/2010

    Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport Timer
{
public:
  Timer();
  virtual ~Timer();
  void restart();
  void resets(const bool);
  double elapsed();
  std::string str() const;
private:
  boost::timer m_timer;
  bool m_resets;
};

DLLExport std::ostream& operator<<(std::ostream&, const Timer);

} // namespace Kernel
} // namespace Mantid

#endif /* TIMER_H_ */
