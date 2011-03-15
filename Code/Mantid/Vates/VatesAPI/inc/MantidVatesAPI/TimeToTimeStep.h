#ifndef MANTID_PARAVIEW_TIME_TO_TIMESTEP
#define MANTID_PARAVIEW_TIME_TO_TIMESTEP

#include "MantidKernel/System.h"
#include <functional>

/** Unary operation applying visualisation platforms specific conversion from a time to a timestep understood by underlying mantid code, where time is treated as an index
 * in a single dimensional array. See MDWorkspace/MDImage.

 @author Owen Arnold, Tessella plc
 @date 14/03/2011

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */

namespace Mantid
{
namespace VATES
{
class DLLExport TimeToTimeStep: std::unary_function<double, int>
{
private:
  double m_timeRange;
  int m_nIntervalSteps;
public:

  TimeToTimeStep(double timeMin, double timeMax, unsigned int nIntervalSteps);
  int operator()(double time) const;

};
}
}

#endif
