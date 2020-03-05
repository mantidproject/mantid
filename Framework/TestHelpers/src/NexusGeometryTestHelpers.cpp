// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidTestHelpers/NexusGeometryTestHelpers.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidNexusGeometry/NexusShapeFactory.h"
#include <numeric>

using namespace Mantid::NexusGeometry;

namespace NexusGeometryTestHelpers {
boost::shared_ptr<const Mantid::Geometry::IObject> createShape() {
  Eigen::Matrix<double, 3, 3> pointsDef;
  // Vector to base positioned at center of cylinder
  pointsDef.col(0) = Eigen::Vector3d(-0.00101, 0.0, 0.0);
  // vector to base which is radial
  pointsDef.col(1) = Eigen::Vector3d(-0.00101, 0.00405, 0.0);
  // vector to top positioned at center of cylinder
  pointsDef.col(2) = Eigen::Vector3d(0.00101, 0.0, 0.0);
  return NexusShapeFactory::createCylinder(pointsDef);
}

Pixels generateCoLinearPixels() {
  // Assuming tubes with axis vectors (1, 0, 0) create two tubes lying along x
  // axis.
  Eigen::Matrix<double, 3, 4> pix;
  double tube1_y = 0;
  double tube2_y = 0.05;

  // Add 2 cylinder positions to tube 1
  for (int i = 0; i < 2; ++i)
    pix.col(i) = Eigen::Vector3d(0.00202 * i, tube1_y, 0);

  // Add 2 cylinder positions to tube 2
  for (int i = 2; i < 4; ++i)
    pix.col(i) = Eigen::Vector3d(0.00202 * i, tube2_y, 0);

  return pix;
}

Pixels generateNonCoLinearPixels() {
  // Add two 4 cylinders which are not CoLinear
  Eigen::Matrix<double, 3, 4> pix;
  pix.col(0) = Eigen::Vector3d(0, 0.1, 0);
  pix.col(1) = Eigen::Vector3d(0.3, 0.6, 0.3);
  pix.col(2) = Eigen::Vector3d(-0.7, -0.7, 0);
  pix.col(3) = Eigen::Vector3d(1, 1.9, 0);

  return pix;
}

std::vector<int> getFakeDetIDs() {
  std::vector<int> detIDs(4);
  std::iota(detIDs.begin(), detIDs.end(), 4);
  return detIDs;
}
} // namespace NexusGeometryTestHelpers
