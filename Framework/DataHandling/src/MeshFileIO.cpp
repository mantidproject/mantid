// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/MeshFileIO.h"

namespace Mantid {
namespace DataHandling {

/**
 * Generates a rotation Matrix applying the x rotation then y rotation, then z
 * rotation
 * @param xRotation The x rotation required in radians
 * @param yRotation The y rotation required in radians
 * @param zRotation The z rotation required in radians
 * @returns a matrix of doubles to use as the rotation matrix
 */
Kernel::Matrix<double> MeshFileIO::generateMatrix(double xRotation,
                                                  double yRotation,
                                                  double zRotation) {
  Kernel::Matrix<double> xMatrix = generateXRotation(xRotation);
  Kernel::Matrix<double> yMatrix = generateYRotation(yRotation);
  Kernel::Matrix<double> zMatrix = generateZRotation(zRotation);

  return zMatrix * yMatrix * xMatrix;
}

/**
 * Rotates the environment by a generated matrix
 * @param environmentMesh The environment to rotate
 * @param xRotation The x rotation required in radians
 * @param yRotation The y rotation required in radians
 * @param zRotation The z rotation required in radians
 * @returns a shared pointer to the newly rotated environment
 */
boost::shared_ptr<Geometry::MeshObject>
MeshFileIO::rotate(boost::shared_ptr<Geometry::MeshObject> environmentMesh,
                   double xRotation, double yRotation, double zRotation) {
  const std::vector<double> rotationMatrix =
      generateMatrix(xRotation, yRotation, zRotation);
  environmentMesh->rotate(rotationMatrix);
  return environmentMesh;
}

/**
 *Generates the x component of the rotation matrix
 *@param xRotation The x rotation required in radians
 *@returns a matrix of doubles to use as the x axis rotation matrix
 */
Kernel::Matrix<double> MeshFileIO::generateXRotation(double xRotation) {
  const double sinX = sin(xRotation);
  const double cosX = cos(xRotation);
  std::vector<double> matrixList = {1, 0, 0, 0, cosX, -sinX, 0, sinX, cosX};
  return Kernel::Matrix<double>(matrixList);
}

/**
 * Generates the y component of the rotation matrix
 * @param yRotation The y rotation required in radians
 * @returns a matrix of doubles to use as the y axis rotation matrix
 */
Kernel::Matrix<double> MeshFileIO::generateYRotation(double yRotation) {
  const double sinY = sin(yRotation);
  const double cosY = cos(yRotation);
  std::vector<double> matrixList = {cosY, 0, sinY, 0, 1, 0, -sinY, 0, cosY};
  return Kernel::Matrix<double>(matrixList);
}

/**
 * Generates the z component of the rotation matrix
 * @param zRotation The z rotation required in radians
 * @returns a matrix of doubles to use as the z axis rotation matrix
 */
Kernel::Matrix<double> MeshFileIO::generateZRotation(double zRotation) {
  const double sinZ = sin(zRotation);
  const double cosZ = cos(zRotation);
  std::vector<double> matrixList = {cosZ, -sinZ, 0, sinZ, cosZ, 0, 0, 0, 1};
  return Kernel::Matrix<double>(matrixList);
}

/**
 * translates the environment by a provided matrix
 * @param environmentMesh The environment to translate
 * @param translationVector The 3D translation to apply
 * @returns a shared pointer to the newly translated environment
 */
boost::shared_ptr<Geometry::MeshObject>
MeshFileIO::translate(boost::shared_ptr<Geometry::MeshObject> environmentMesh,
                      const std::vector<double> translationVector) {
  std::vector<double> checkVector = std::vector<double>(3, 0.0);
  if (translationVector != checkVector) {
    if (translationVector.size() != 3) {
      throw std::invalid_argument(
          "Invalid Translation vector, must have exactly 3 dimensions");
    }
    Kernel::V3D translate = createScaledV3D(
        translationVector[0], translationVector[1], translationVector[2]);
    environmentMesh->translate(translate);
  }
  return environmentMesh;
}

/**
 * scales a 3D point according the units defined in the MeshFileIO class
 * @param xVal The x coordinate to be scaled
 * @param yVal The y coordinate to be scaled
 * @param zVal The z coordinate to be scaled
 * @returns a 3D vector containing the scaled coordinates
 */
Kernel::V3D MeshFileIO::createScaledV3D(double xVal, double yVal, double zVal) {
  xVal = scaleValue(xVal);
  yVal = scaleValue(yVal);
  zVal = scaleValue(zVal);
  return Kernel::V3D(double(xVal), double(yVal), double(zVal));
}

} // namespace DataHandling
} // namespace Mantid