// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** GenerateIPythonNotebook

  An Algorithm to generate an IPython notebook file to record the history of a
  workspace.

  Properties:
  <ul>
  <li>Filename - the name of the file to write to. </li>
  <li>InputWorkspace - the workspace name who's history is to be saved.</li>
  </ul>
*/
class MANTID_ALGORITHMS_DLL GenerateIPythonNotebook final : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "GenerateIPythonNotebook"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "An Algorithm to generate an IPython notebook file to record the "
           "history of a workspace.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"GeneratePythonScript"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Utility\\Python"; }

protected:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
