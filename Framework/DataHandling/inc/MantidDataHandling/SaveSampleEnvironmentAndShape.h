// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef DATAHANDLING_SAVE_ENVIRONMENT_H_
#define DATAHANDLING_SAVE_ENVIRONMENT_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/Matrix.h"

namespace Mantid {
namespace Geometry {
class MeshObject;
}
namespace DataHandling {
/**
 * Save the Shape of the sample and environment into a single binary .stl file
 */

class DLLExport SaveSampleEnvironmentAndShape : public Mantid::API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SaveSampleEnvironmentAndShape"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The algorithm saves the environment and sample shape from the instrument of a "
           "workspace. ";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Related algorithms
  const std::vector<std::string> seeAlso() const override {
    return {"LoadSampleEnvironment", "SetSampleMaterial", "LoadSampleShape"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "DataHandling\\Instrument";
  }

private:
  // Implement abstract Algorithm methods
  void init() override;
  void exec() override;  
};

} // end namespace DataHandling
} // namespace Mantid

#endif /* DATAHANDLING_SAVE_ENVIRONMENT_H_ */
