// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataHandling {

/** SaveMaskingToFile : TODO: DESCRIPTION

  @date 2011-11-09
*/
class DLLExport SaveMask : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "SaveMask"; };
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Save a MaskWorkspace/SpecialWorkspace2D to an XML file."; }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"SaveMask", "LoadMask"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Masking;Transforms\\Masking"; }

private:
  /// Define input parameters
  void init() override;

  /// Main body to execute algorithm
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid
