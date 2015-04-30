#ifndef MANTID_MDALGORITHMS_REFLECTOMETRYMDTRANFORM_H_
#define MANTID_MDALGORITHMS_REFLECTOMETRYMDTRANFORM_H_

#include "MantidAPI/BoxController.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"

#include "MantidDataObjects/MDEventFactory.h"

namespace Mantid {
namespace API {
class MatrixWorkspace;
}
namespace MDAlgorithms {

/** ReflectometryMDTransform : Abstract type for reflectometry transforms to
 MDWorkspaces. This is a Strategy Design Pattern.

 @date 2012-05-29

 Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport ReflectometryTransform {

protected:
  const size_t m_nbinsx;
  const size_t m_nbinsz;

  boost::shared_ptr<DataObjects::MDEventWorkspace2Lean>
  createMDWorkspace(Geometry::IMDDimension_sptr,
                    Geometry::IMDDimension_sptr,
                    API::BoxController_sptr boxController) const;

public:
  // Execute the strategy to produce a transformed, output MDWorkspace
  virtual Mantid::API::IMDEventWorkspace_sptr
  executeMD(Mantid::API::MatrixWorkspace_const_sptr inputWs,
            Mantid::API::BoxController_sptr boxController) const = 0;

  // Execute the strategy to produce a transformed, output group of Matrix (2D)
  // Workspaces
  virtual Mantid::API::MatrixWorkspace_sptr
  execute(Mantid::API::MatrixWorkspace_const_sptr inputWs) const = 0;

  virtual ~ReflectometryTransform();
  ReflectometryTransform(int numberOfBinsQx, int numberOfBinsQz);
};

/// Create a new x-axis for the output workspace
MantidVec createXAxis(Mantid::API::MatrixWorkspace *const ws,
                      const double gradQx, const double cxToUnit,
                      const size_t nBins, const std::string &caption,
                      const std::string &units);

/// Create a new y(vertical)-axis for the output workspace
void createVerticalAxis(Mantid::API::MatrixWorkspace *const ws,
                        const MantidVec &xAxisVec, const double gradQz,
                        const double cyToUnit, const size_t nBins,
                        const std::string &caption, const std::string &units);

// Helper typedef for scoped pointer of this type.
typedef boost::shared_ptr<ReflectometryTransform> ReflectometryTransform_sptr;
}
}
#endif
