// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/**
  Extract the masking from a given workspace.

  The output workspce is a MaskWorkspace with a single X bin where:
  <UL>
  <LI>1 = masked;</LI>
  <LI>0 = unmasked.</LI>
  </UL>

  The spectra containing 0 are also marked as masked and the instrument
  link is preserved so that the instrument view functions correctly.

  Required Properties:
  <UL>
  <LI> InputWorkspace  - The name of the input workspace. </LI>
  <LI> OutputWorkspace - The name of the output mask workspace </LI>
  </UL>
*/
class MANTID_ALGORITHMS_DLL ExtractMask : public Mantid::API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "ExtractMask"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Extracts the masking from a given workspace and places it in a new "
           "workspace.";
  }

  /// Algorithm's version
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"ExtractMaskToTable"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Transforms\\Masking"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};
} // namespace Algorithms
} // namespace Mantid
