#ifndef MANTID_PARAVIEW_THRESHOLD_RANGE
#define MANTID_PARAVIEW_THRESHOLD_RANGE

#include <boost/shared_ptr.hpp>
#include "MantidKernel/System.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidAPI/Workspace.h"

/** Abstract type promises to supply a minimum and maximum set of threshold range values.

 @author Owen Arnold, Tessella plc
 @date 30/06/2011

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
class DLLExport ThresholdRange 
{

public:
  
  /// Calculate the threshold range.
  virtual void calculate() = 0;

  /// Getter for the has executed status.
  virtual bool hasCalculated() const = 0;

  /// Fetch the threshold range.
  virtual signal_t getMinimum() const = 0;

  /// Fetch the threshold range minimum.
  virtual signal_t getMaximum() const = 0;

  /// Virtual constructor method.
  virtual ThresholdRange* clone() const = 0;

  /// Determine wheter the given value is within the range.
  virtual bool inRange(const signal_t& signal) = 0;

  /// Destructor
  virtual ~ThresholdRange()
  {
  }

  /// Interface allows the threshold range to accept a workspace.
  virtual void setWorkspace(Mantid::API::Workspace_sptr)
  {
  }
};

/// ThresholdRange as a scoped pointer.
typedef boost::shared_ptr<ThresholdRange> ThresholdRange_scptr;
}
}

#endif
