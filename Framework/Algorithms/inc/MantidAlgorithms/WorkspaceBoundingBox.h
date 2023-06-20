// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace Algorithms {
namespace {
// static logger
Kernel::Logger g_log("WorkspaceBoundingBox");
} // namespace

/* This is a simple class originally intended for use solely with FindCenterOfMassPosition2.cpp
 *
 */
class WorkspaceBoundingBox {
public:
  WorkspaceBoundingBox(const API::MatrixWorkspace_const_sptr &workspace, const double beamRadius,
                       const bool ignoreDirectBeam);
  WorkspaceBoundingBox();
  ~WorkspaceBoundingBox();

  API::MatrixWorkspace_const_sptr getWorkspace() { return m_workspace; }
  double getX() const { return m_centerXPosCurr; }
  double getY() const { return m_centerYPosCurr; }
  double getCenterX() const { return m_centerXPosPrev; }
  double getCenterY() const { return m_centerYPosPrev; }
  double getXMin() const { return m_xPosMin; }
  double getXMax() const { return m_xPosMax; }
  double getYMin() const { return m_yPosMin; }
  double getYMax() const { return m_yPosMax; }

  void setCenter(const double x, const double y);
  void setBounds(const double xMin, const double xMax, const double yMin, const double yMax);

  double calculateDistance() const;
  double calculateRadiusX() const;
  double calculateRadiusY() const;

  void initBoundingBox();
  void updateBoundingBox();
  bool symmetricRegionContainsPoint(double x, double y);
  void normalizePosition(const double totalCounts);

private:
  Kernel::V3D position(const std::size_t index) const;
  void resetIntermediatePosition();
  double countsValue(const std::size_t index) const;
  bool isValidIndex(const std::size_t index) const;
  void updateMinMax(const std::size_t index);
  bool includeInIntegration(const std::size_t index);
  bool includeInIntegration(const Kernel::V3D &position);
  double updatePositionAndReturnCount(const std::size_t index);
  API::MatrixWorkspace_const_sptr m_workspace;
  const API::SpectrumInfo *m_spectrumInfo;
  std::size_t m_numSpectra;
  double m_beamRadiusSq;
  bool m_ignoreDirectBeam;
  double m_centerXPosCurr{0}; // intermediate value
  double m_centerYPosCurr{0}; // intermediate value
  double m_centerXPosPrev{0};
  double m_centerYPosPrev{0};
  // overall range to consider
  double m_xPosMin{0};
  double m_xPosMax{0};
  double m_yPosMin{0};
  double m_yPosMax{0};
  // range for current search
  double m_xBoxMin{0};
  double m_xBoxMax{0};
  double m_yBoxMin{0};
  double m_yBoxMax{0};
};

} // namespace Algorithms
} // namespace Mantid
