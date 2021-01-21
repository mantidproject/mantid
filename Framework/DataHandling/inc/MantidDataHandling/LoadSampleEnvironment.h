// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MeshFileIO.h"

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
  const std::string category() const override { return "DataHandling\\Instrument"; }
  std::map<std::string, std::string> validateInputs() override;

private:
  // Implement abstract Algorithm methods
  void init() override;
  void exec() override;

  void loadEnvironmentFromSTL(const std::string filename, API::Sample &sample, const bool add, std::string debugString);

  void loadEnvironmentFrom3MF(API::MatrixWorkspace_const_sptr inputWS, const std::string filename, API::Sample &sample,
                              const bool add, std::string debugString);
};

} // end namespace DataHandling
} // namespace Mantid
