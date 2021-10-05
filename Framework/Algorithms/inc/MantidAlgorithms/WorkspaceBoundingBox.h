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
  WorkspaceBoundingBox(const API::MatrixWorkspace_const_sptr &workspace);
  WorkspaceBoundingBox();
  ~WorkspaceBoundingBox();

  API::MatrixWorkspace_const_sptr getWorkspace() { return m_workspace; }
  double getX() const { return x; }
  double getY() const { return y; }
  double getCenterX() const { return centerX; }
  double getCenterY() const { return centerY; }
  double getXMin() const { return xMin; }
  double getXMax() const { return xMax; }
  double getYMin() const { return yMin; }
  double getYMax() const { return yMax; }

  void setPosition(double x, double y);
  void setCenter(double x, double y);
  void setBounds(double xMin, double xMax, double yMin, double yMax);

  double calculateDistance() const;
  double calculateRadiusX() const;
  double calculateRadiusY() const;

  double updatePositionAndReturnCount(int index);
  int findFirstValidWs(const int numSpec) const;
  bool isValidWs(int index) const;
  bool isOutOfBoundsOfNonDirectBeam(const double beamRadius, int index, const bool directBeam);
  bool containsPoint(double x, double y);
  void normalizePosition(double x, double y);
  void updateMinMax(int index);

private:
  Kernel::V3D &position(int index) const;
  double yValue(const int index) const;
  API::MatrixWorkspace_const_sptr m_workspace;
  const API::SpectrumInfo *m_spectrumInfo;
  double x{0};
  double y{0};
  double centerX{0};
  double centerY{0};
  double xMin{0};
  double xMax{0};
  double yMin{0};
  double yMax{0};
  // cache information
  mutable int m_cachedPositionIndex{-1};
  mutable Kernel::V3D m_cachedPosition;
  mutable int m_cachedHistogramYIndex{-1};
  mutable double m_cachedYValue;
};

} // namespace Algorithms
} // namespace Mantid
