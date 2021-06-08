// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataHandling {

/** Modifies an ISIS detector dot data file, so that the detector positions are
  as in the given workspace.

  @author Karl Palmen
  @date 2012-08-23
*/
class DLLExport ModifyDetectorDotDatFile : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "ModifyDetectorDotDatFile"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Modifies an ISIS detector dot data file, so that the detector "
           "positions are as in the given workspace";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"ResizeRectangularDetector"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Instrument"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid
