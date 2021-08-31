// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/Logger.h"

namespace Mantid {
namespace Algorithms {

/* This is a simple class originally intended for use solely with FindCenterOfMassPosition2.cpp
 *
 */
class WorkspaceBoundingBox {
public:
  WorkspaceBoundingBox(API::MatrixWorkspace_sptr workspace, Kernel::Logger &g_log);
  WorkspaceBoundingBox(Kernel::Logger &g_log);
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
  bool isOutOfBoundsOfNonDirectBeam(const double beam_radius, int index, const bool direct_beam);
  bool containsPoint(double x, double y);
  void normalizePosition(double x, double y);
  void updateMinMax(int index);

private:
  API::MatrixWorkspace_sptr workspace;
  double x = 0;
  double y = 0;
  double centerX;
  double centerY;
  double xMin = 0;
  double xMax = 0;
  double yMin = 0;
  double yMax = 0;
  const int m_specID = 0;

  /// Logger for this helper
  Kernel::Logger &g_log;
};

} // namespace Algorithms
} // namespace Mantid