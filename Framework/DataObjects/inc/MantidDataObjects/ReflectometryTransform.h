#ifndef MANTID_DATAOBJECTS_REFLECTOMETRYMDTRANFORM_H_
#define MANTID_DATAOBJECTS_REFLECTOMETRYMDTRANFORM_H_

#include "MantidAPI/BoxController.h"
#include "MantidAPI/IEventWorkspace_fwd.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidDataObjects/DllConfig.h"

#include "MantidDataObjects/MDEventFactory.h"
#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace API {
class MatrixWorkspace;
}
namespace DataObjects {

class CalculateReflectometry;
class TableWorkspace;

/**
Simple container for porting detector angular information
 */
struct MANTID_DATAOBJECTS_DLL DetectorAngularCache {
  std::vector<double> thetaWidths;
  std::vector<double> thetas;
  std::vector<double> detectorHeights;
};

/** ReflectometryMDTransform : Base type for reflectometry transforms to
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
class MANTID_DATAOBJECTS_DLL ReflectometryTransform {

protected:
  const size_t m_d0NumBins;
  const size_t m_d1NumBins;
  const double m_d0Min;
  const double m_d1Min;
  const double m_d0Max;
  const double m_d1Max;
  const std::string m_d0Label;
  const std::string m_d1Label;
  const std::string m_d0ID;
  const std::string m_d1ID;
  boost::shared_ptr<CalculateReflectometry> m_calculator;

  /// Two theta angles cache
  mutable std::vector<double> m_theta;
  /// Two theta widths cache
  mutable std::vector<double> m_thetaWidths;

  boost::shared_ptr<DataObjects::MDEventWorkspace2Lean>
  createMDWorkspace(Geometry::IMDDimension_sptr, Geometry::IMDDimension_sptr,
                    API::BoxController_sptr boxController) const;

public:
  // Execute the strategy to produce a transformed, output MDWorkspace
  Mantid::API::IMDEventWorkspace_sptr
  executeMD(Mantid::API::MatrixWorkspace_const_sptr inputWs,
            Mantid::API::BoxController_sptr boxController,
            Mantid::Geometry::MDFrame_uptr frame) const;

  // Execute the strategy to produce a transformed, output group of Matrix (2D)
  // Workspaces
  Mantid::API::MatrixWorkspace_sptr
  execute(Mantid::API::MatrixWorkspace_const_sptr inputWs) const;

  /// Execuate transformation using normalised polynomial binning
  Mantid::API::MatrixWorkspace_sptr executeNormPoly(
      const Mantid::API::MatrixWorkspace_const_sptr &inputWS,
      boost::shared_ptr<Mantid::DataObjects::TableWorkspace> &vertexes,
      bool dumpVertexes, std::string outputDimensions) const;

  Mantid::API::IMDHistoWorkspace_sptr
  executeMDNormPoly(Mantid::API::MatrixWorkspace_const_sptr inputWs) const;
  virtual ~ReflectometryTransform() = default;
  ReflectometryTransform(const std::string &d0Label, const std::string &d0ID,
                         double d0Min, double d0Max, const std::string &d1Label,
                         const std::string &d1ID, double d1Min, double d1Max,
                         size_t d0NumBins, size_t d1NumBins,
                         CalculateReflectometry *calc);
};

/// Create a new x-axis for the output workspace
MANTID_DATAOBJECTS_DLL MantidVec
createXAxis(Mantid::API::MatrixWorkspace *const ws, const double gradX,
            const double cxToUnit, const size_t nBins,
            const std::string &caption, const std::string &units);

/// Create a new y(vertical)-axis for the output workspace
MANTID_DATAOBJECTS_DLL void
createVerticalAxis(Mantid::API::MatrixWorkspace *const ws,
                   const MantidVec &xAxisVec, const double gradY,
                   const double cyToUnit, const size_t nBins,
                   const std::string &caption, const std::string &units);

/// Create angular caches.
MANTID_DATAOBJECTS_DLL DetectorAngularCache
initAngularCaches(const Mantid::API::MatrixWorkspace *const workspace);

// Helper typedef for scoped pointer of this type.
using ReflectometryTransform_sptr = boost::shared_ptr<ReflectometryTransform>;
} // namespace DataObjects
} // namespace Mantid
#endif
