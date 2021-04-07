// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/BoxController.h"
#include "MantidAPI/IEventWorkspace_fwd.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidDataObjects/DllConfig.h"

#include "MantidDataObjects/MDEventFactory.h"
#include <memory>

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
  std::vector<double> twoThetaWidths;
  std::vector<double> twoThetas;
  std::vector<double> detectorHeights;
};

/** ReflectometryMDTransform : Base type for reflectometry transforms to
 MDWorkspaces. This is a Strategy Design Pattern.

 @date 2012-05-29
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
  std::shared_ptr<CalculateReflectometry> m_calculator;

  std::shared_ptr<DataObjects::MDEventWorkspace2Lean>
  createMDWorkspace(const Geometry::IMDDimension_sptr &, const Geometry::IMDDimension_sptr &,
                    const API::BoxController_sptr &boxController) const;

public:
  // Execute the strategy to produce a transformed, output MDWorkspace
  Mantid::API::IMDEventWorkspace_sptr executeMD(const Mantid::API::MatrixWorkspace_const_sptr &inputWs,
                                                const Mantid::API::BoxController_sptr &boxController,
                                                Mantid::Geometry::MDFrame_uptr frame) const;

  // Execute the strategy to produce a transformed, output group of Matrix (2D)
  // Workspaces
  Mantid::API::MatrixWorkspace_sptr execute(const Mantid::API::MatrixWorkspace_const_sptr &inputWs) const;

  /// Execuate transformation using normalised polynomial binning
  Mantid::API::MatrixWorkspace_sptr executeNormPoly(const Mantid::API::MatrixWorkspace_const_sptr &inputWS,
                                                    std::shared_ptr<Mantid::DataObjects::TableWorkspace> &vertexes,
                                                    bool dumpVertexes, const std::string &outputDimensions) const;

  Mantid::API::IMDHistoWorkspace_sptr executeMDNormPoly(const Mantid::API::MatrixWorkspace_const_sptr &inputWs) const;
  virtual ~ReflectometryTransform() = default;
  ReflectometryTransform(const std::string &d0Label, const std::string &d0ID, double d0Min, double d0Max,
                         const std::string &d1Label, const std::string &d1ID, double d1Min, double d1Max,
                         size_t d0NumBins, size_t d1NumBins, CalculateReflectometry *calc);
};

/// Create a new x-axis for the output workspace
MANTID_DATAOBJECTS_DLL MantidVec createXAxis(Mantid::API::MatrixWorkspace *const ws, const double gradX,
                                             const double cxToUnit, const size_t nBins, const std::string &caption,
                                             const std::string &units);

/// Create a new y(vertical)-axis for the output workspace
MANTID_DATAOBJECTS_DLL void createVerticalAxis(Mantid::API::MatrixWorkspace *const ws, const MantidVec &xAxisVec,
                                               const double gradY, const double cyToUnit, const size_t nBins,
                                               const std::string &caption, const std::string &units);

/// Create angular caches.
MANTID_DATAOBJECTS_DLL DetectorAngularCache initAngularCaches(const Mantid::API::MatrixWorkspace *const workspace);

// Helper typedef for scoped pointer of this type.
using ReflectometryTransform_sptr = std::shared_ptr<ReflectometryTransform>;
} // namespace DataObjects
} // namespace Mantid
