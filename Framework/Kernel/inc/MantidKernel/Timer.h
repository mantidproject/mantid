#ifndef MANTID_KERNEL_TIMER_H_
#define MANTID_KERNEL_TIMER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include <iosfwd>
#include <string>

#ifdef _WIN32
#include <time.h>
#else
#include <sys/time.h>
#endif
#include <string>

namespace Mantid {
namespace Kernel {
/** A simple class that provides a wall-clock (not processor time) timer.

    @author Russell Taylor, Tessella plc
    @date 29/04/2010

    Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

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
class MANTID_KERNEL_DLL Timer {
public:
  Timer();
  virtual ~Timer();

  float elapsed(bool reset = true);
  float elapsed_no_reset() const;
  std::string str() const;
  void reset();

private:
// The type of this variable is different depending on the platform
#ifdef _WIN32
  clock_t
#else
  timeval
#endif
      m_start; ///< The starting time (implementation dependent format)
};

std::ostream &operator<<(std::ostream &, const Timer &);

} // namespace Kernel
} // namespace Mantid

#endif /* TIMER_H_ */
