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
  WorkspaceBoundingBox(API::MatrixWorkspace_sptr workspace);
  WorkspaceBoundingBox();
  ~WorkspaceBoundingBox();

  API::MatrixWorkspace_sptr getWorkspace() { return workspace; }
  double getX() { return x; }
  double getY() { return y; }
  double getCenterX() { return centerX; }
  double getCenterY() { return centerY; }
  double getXMin() { return xMin; }
  double getXMax() { return xMax; }
  double getYMin() { return yMin; }
  double getYMax() { return yMax; }

  void setPosition(double x, double y);
  void setCenter(double x, double y);
  void setBounds(double xMin, double xMax, double yMin, double yMax);

  double calculateDistance();
  double calculateRadiusX();
  double calculateRadiusY();

  double updatePositionAndReturnCount(int index);
  int findFirstValidWs(const int numSpec);
  bool isValidWs(int index);
  bool isOutOfBoundsOfNonDirectBeam(const double beamRadius, int index, const bool directBeam);
  bool containsPoint(double x, double y);
  void normalizePosition(double x, double y);
  void updateMinMax(int index);

private:
  Kernel::V3D &position(int index);
  const HistogramData::HistogramY &histogramY(int index);
  API::MatrixWorkspace_sptr workspace;
  const API::SpectrumInfo *spectrumInfo;
  double x = 0;
  double y = 0;
  double centerX;
  double centerY;
  double xMin = 0;
  double xMax = 0;
  double yMin = 0;
  double yMax = 0;
  const int m_specID = 0;
  int m_cachedPositionIndex = -1;
  int m_cachedHistogramYIndex = -1;
  Kernel::V3D m_cachedPosition;
  const HistogramData::HistogramY *m_cachedHistogramY;
};

} // namespace Algorithms
} // namespace Mantid
