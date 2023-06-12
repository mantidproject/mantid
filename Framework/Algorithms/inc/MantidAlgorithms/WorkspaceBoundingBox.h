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
  double getX() const { return m_xPos; }
  double getY() const { return m_yPos; }
  double getCenterX() const { return m_centerXPos; }
  double getCenterY() const { return m_centerYPos; }
  double getXMin() const { return m_xPosMin; }
  double getXMax() const { return m_xPosMax; }
  double getYMin() const { return m_yPosMin; }
  double getYMax() const { return m_yPosMax; }

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
  double m_xPos{0};
  double m_yPos{0};
  double m_centerXPos{0};
  double m_centerYPos{0};
  double m_xPosMin{0};
  double m_xPosMax{0};
  double m_yPosMin{0};
  double m_yPosMax{0};
  // cache information
  mutable int m_cachedPositionIndex{-1};
  mutable Kernel::V3D m_cachedPosition;
  mutable int m_cachedHistogramYIndex{-1};
  mutable double m_cachedYValue;
};

} // namespace Algorithms
} // namespace Mantid
