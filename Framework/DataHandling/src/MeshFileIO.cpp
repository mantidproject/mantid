// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/MeshFileIO.h"
#include "MantidGeometry/Objects/ShapeFactory.h"

namespace Mantid::DataHandling {

/**
 * Rotates the environment by a generated matrix
 * @param environmentMesh The environment to rotate
 * @param xRotation The x rotation required in radians
 * @param yRotation The y rotation required in radians
 * @param zRotation The z rotation required in radians
 * @returns a shared pointer to the newly rotated environment
 */
std::shared_ptr<Geometry::MeshObject> MeshFileIO::rotate(std::shared_ptr<Geometry::MeshObject> environmentMesh,
                                                         double xRotation, double yRotation, double zRotation) {
  const std::vector<double> rotationMatrix = Geometry::ShapeFactory::generateMatrix(xRotation, yRotation, zRotation);
  environmentMesh->rotate(rotationMatrix);
  return environmentMesh;
}

/**
 * translates the environment by a provided matrix
 * @param environmentMesh The environment to translate
 * @param translationVector The 3D translation to apply
 * @returns a shared pointer to the newly translated environment
 */
std::shared_ptr<Geometry::MeshObject> MeshFileIO::translate(std::shared_ptr<Geometry::MeshObject> environmentMesh,
                                                            const std::vector<double> &translationVector) {
  std::vector<double> checkVector = std::vector<double>(3, 0.0);
  if (translationVector != checkVector) {
    if (translationVector.size() != 3) {
      throw std::invalid_argument("Invalid Translation vector, must have exactly 3 dimensions");
    }
    Kernel::V3D scaledTranslationVector =
        createScaledV3D(translationVector[0], translationVector[1], translationVector[2]);
    environmentMesh->translate(scaledTranslationVector);
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

} // namespace Mantid::DataHandling
