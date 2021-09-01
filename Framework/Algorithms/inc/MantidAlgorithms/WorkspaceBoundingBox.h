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
  /** Sets member variables x/y to new x/y based on
   *  spectrum info and historgram data at the given index
   *
   *  @param index :: index of spectrum data
   *  @return number of points of histogram data at index
   */
  double updatePositionAndReturnCount(int index);
  /** Searches for the first valid spectrum info in member variable `workspace`
   *
   *  @param numSpec :: the number of spectrum in the workspace to search through
   *  @return index of first valid spectrum
   */
  int findFirstValidWs(const int numSpec);
  /** Performs checks on the spectrum located at index to determine if
   *  it is acceptable to be operated on
   *
   *  @param index :: index of spectrum data
   *  @return true/false if its valid
   */
  bool isValidWs(int index);
  /** Checks to see if spectrum at index is within the diameter of the given beamRadius
   *
   *  @param beamRadius :: radius of beam in meters
   *  @param index :: index of spectrum data
   *  @param directBeam :: whether or not the spectrum is subject to the beam
   *  @return number of points of histogram data at index
   */
  bool isOutOfBoundsOfNonDirectBeam(const double beamRadius, int index, const bool directBeam);
  /** Checks if a given x/y coord is within the bounding box
   *
   *  @param x :: x coordinate
   *  @param y :: y coordinate
   *  @return true/false if it is within the mins/maxs of the box
   */
  bool containsPoint(double x, double y);
  /** Perform normalization on x/y coords over given values
   *
   *  @param x :: value to normalize member x over
   *  @param y :: value to normalize member y over
   */
  void normalizePosition(double x, double y);
  /** Compare current mins and maxs to the coordinates of the spectrum at index
   *  expnd mins and maxs to include this spectrum
   *
   *  @param index :: index of spectrum data
   */
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