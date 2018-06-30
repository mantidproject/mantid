#ifndef IMD_DIMENSION_COMPARITOR_H
#define IMD_DIMENSION_COMPARITOR_H

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"

namespace Mantid {
namespace VATES {

/** Dimension comparitor specifically for use with visualisation layer. Given an
arrangement of dimensions in an MDImage, this type
allow the utilising code to ask wheter some dimension maps to the x, y, or z
dimensions.

@author Owen Arnold, Tessella plc
@date 25/03/2011

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
class IMDDimensionComparitor {
public:
  /// Constructor
  IMDDimensionComparitor(Mantid::API::IMDWorkspace_sptr workspace);
  IMDDimensionComparitor operator=(IMDDimensionComparitor &) = delete;
  IMDDimensionComparitor(IMDDimensionComparitor &) = delete;

  /// Destructor
  ~IMDDimensionComparitor();

  bool isXDimension(const Mantid::Geometry::IMDDimension &queryDimension);

  bool isYDimension(const Mantid::Geometry::IMDDimension &queryDimension);

  bool isZDimension(const Mantid::Geometry::IMDDimension &queryDimension);

  bool istDimension(const Mantid::Geometry::IMDDimension &queryDimension);

private:
  /// imd workspace shared ptr.
  Mantid::API::IMDWorkspace_sptr m_workspace;
};
} // namespace VATES
} // namespace Mantid

#endif
