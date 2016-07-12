#ifndef MANTID_PARAVIEW_USER_DEFINED_THRESHOLD_RANGE
#define MANTID_PARAVIEW_USER_DEFINED_THRESHOLD_RANGE

#include "MantidKernel/System.h"
#include "MantidVatesAPI/ThresholdRange.h"
/** Stores range values specified by the user.

 @author Owen Arnold, Tessella plc
 @date 30/06/2011

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

 File change history is stored at: <https://github.com/mantidproject/mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */

namespace Mantid {
namespace VATES {
class DLLExport UserDefinedThresholdRange : public ThresholdRange {

public:
  UserDefinedThresholdRange(signal_t min, signal_t max);

  void calculate() override;

  signal_t getMinimum() const override;

  signal_t getMaximum() const override;

  ~UserDefinedThresholdRange() override;

  bool hasCalculated() const override;

  UserDefinedThresholdRange *clone() const override;

  bool inRange(const signal_t &signal) override;

private:
  const signal_t m_min;

  const signal_t m_max;
};
}
}

#endif
