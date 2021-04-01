// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Sample.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** CopySample : The algorithm copies some/all the sample information from one
workspace to another.
For MD workspaces, if no input sample number is specified, or not found, it will
copy the first sample.
For MD workspaces, if no output sample number is specified (or negative), it
will copy to all samples. The following information can be copied:

  Name
  Material
  Sample environment
  Shape
  Oriented lattice

  @author Andrei Savici, ORNL
  @date 2011-08-11
*/
class MANTID_ALGORITHMS_DLL CopySample : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "CopySample"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Copy some/all the sample information from one workspace to "
           "another.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"CompareSampleLogs", "CopyLogs", "CheckForSampleLogs"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Sample;Utility\\Workspaces"; }
  std::map<std::string, std::string> validateInputs() override;

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
  /// Function to copy information from one sample to another
  void copyParameters(API::Sample &from, API::Sample &to, bool nameFlag, bool materialFlag, bool environmentFlag,
                      bool shapeFlag, bool latticeFlag, bool orientationOnlyFlag);
};

} // namespace Algorithms
} // namespace Mantid
