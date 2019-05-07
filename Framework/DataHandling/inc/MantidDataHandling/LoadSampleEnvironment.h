// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef DATAHANDLING_LOAD_ENVIRONMENT_H_
#define DATAHANDLING_LOAD_ENVIRONMENT_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/Matrix.h"
#include "LoadShape.h"

namespace Mantid {
namespace Geometry {
class MeshObject;
}
namespace DataHandling {
/**
 * Load Environment into the sample of a workspace, either replacing the
 * current environment, or replacing it, you may also set the material
 *
 * The following file types are supported:
 *   - STL file with suffix .stl
 */

class DLLExport LoadSampleEnvironment : public Mantid::API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadSampleEnvironment"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The algorithm loads an Environment into the instrument of a "
           "workspace "
           "at the sample.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Related algorithms
  const std::vector<std::string> seeAlso() const override {
    return {"CopySample", "SetSampleMaterial", "LoadSampleShape"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "DataHandling\\Instrument";
  }

  boost::shared_ptr<Geometry::MeshObject>
  translate(boost::shared_ptr<Geometry::MeshObject> environmentMesh,ScaleUnits scaleType);
  boost::shared_ptr<Geometry::MeshObject>
  rotate(boost::shared_ptr<Geometry::MeshObject> environmentMesh);

  Kernel::Matrix<double> generateMatrix();
  Kernel::Matrix<double> generateXRotation();
  Kernel::Matrix<double> generateYRotation();
  Kernel::Matrix<double> generateZRotation();
  std::map<std::string, std::string> validateInputs() override;

private:
  // Implement abstract Algorithm methods
  void init() override;
  void exec() override;
  Kernel::V3D createScaledV3D(double xVal, double yVal, double zVal,ScaleUnits scaleType);
  
};

} // end namespace DataHandling
} // namespace Mantid

#endif /* DATAHANDLING_LOAD_ENVIRONMENT_H_ */
