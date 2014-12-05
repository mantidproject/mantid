#ifndef MANTID_PARAVIEW_TIMESTEP_TO_TIMESTEP
#define MANTID_PARAVIEW_TIMESTEP_TO_TIMESTEP

#include "MantidKernel/System.h"
#include <functional>

/** Maps from a timestep to a timestep. Provides the static compile time polymorphism required by vtkDataSetFactory type classes.

 @author Owen Arnold, Tessella plc
 @date 14/03/2011

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

 File change history is stored at: <https://github.com/mantidproject/mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */

namespace Mantid
{
namespace VATES
{
class DLLExport TimeStepToTimeStep: std::unary_function<int, int>
{
private:
  double m_timeRange;

  size_t m_nIntervalSteps;
  
  TimeStepToTimeStep(double timeMin, double timeMax, size_t intervalStep);

public:

  /// Constructional method.
  static TimeStepToTimeStep construct(double timeMin, double timeMax, size_t nIntervalSteps);

  TimeStepToTimeStep();

  size_t operator()(double timeStep) const;

};
}
}

#endif
