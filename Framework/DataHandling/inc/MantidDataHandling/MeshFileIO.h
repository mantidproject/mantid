// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"

#ifndef MANTID_DATAHANDLING_MESHFILEIO_H_
#define MANTID_DATAHANDLING_MESHFILEIO_H_
namespace {
Mantid::Kernel::Logger g_log("MeshFileIO");
}
namespace Mantid {
namespace DataHandling {
enum class ScaleUnits { undefined, metres, centimetres, millimetres };

class DLLExport MeshFileIO {
public:
  Kernel::Matrix<double> generateMatrix(double xRotation, double yRotation,
                                        double zRotation);

  boost::shared_ptr<Geometry::MeshObject>
  rotate(boost::shared_ptr<Geometry::MeshObject> environmentMesh,
         double xRotation, double yRotation, double zRotation);

  Kernel::Matrix<double> generateXRotation(double xRotation);
  Kernel::Matrix<double> generateYRotation(double yRotation);
  Kernel::Matrix<double> generateZRotation(double zRotation);

  boost::shared_ptr<Geometry::MeshObject>
  translate(boost::shared_ptr<Geometry::MeshObject> environmentMesh,
            const std::vector<double> translationVector);

  Kernel::V3D createScaledV3D(double xVal, double yVal, double zVal);

protected:
  MeshFileIO(ScaleUnits scaleType) : m_scaleType(scaleType) {}
  MeshFileIO(ScaleUnits scaleType, std::vector<uint32_t> triangles,
             std::vector<Kernel::V3D> vertices)
      : m_scaleType(scaleType), m_triangle(triangles), m_vertices(vertices) {}
  double scaleValue(double val) {
    switch (m_scaleType) {
    case ScaleUnits::centimetres:
      return val / 100;
    case ScaleUnits::millimetres:
      return val / 1000;
    default:
      return val;
    }
  }

  float removeScale(double value) {
    switch (m_scaleType) {
    case ScaleUnits::centimetres:
      value = value * 100;
      break;
    case ScaleUnits::millimetres:
      value = value * 1000;
      break;
    default:; // do nothing
    }
    return float(value);
  };

  ScaleUnits m_scaleType;
  std::vector<uint32_t> m_triangle;
  std::vector<Kernel::V3D> m_vertices;

  void setScaleType(const ScaleUnits scaleType) {
    if (m_scaleType == ScaleUnits::undefined) {
      m_scaleType = scaleType;
    }
  }
};
inline ScaleUnits getScaleType(const std::string &scaleProperty) {
  ScaleUnits scaleType;
  if (scaleProperty == "m") {
    scaleType = ScaleUnits::metres;
  } else if (scaleProperty == "cm") {
    scaleType = ScaleUnits::centimetres;
  } else if (scaleProperty == "mm") {
    scaleType = ScaleUnits::millimetres;
  } else {
    throw std::invalid_argument(
        scaleProperty + " is not an accepted scale of stl or 3mf file.");
  }
  return scaleType;
}

} // namespace DataHandling
} // namespace Mantid
#endif /*MANTID_DATAHANDLING_MESHFILEIO_H_*/