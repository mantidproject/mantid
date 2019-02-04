// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_EXTRACTSINGLESPECTRUM_H_
#define MANTID_ALGORITHMS_EXTRACTSINGLESPECTRUM_H_

#include "MantidAPI/DistributedAlgorithm.h"

namespace Mantid {
namespace Algorithms {
/** Copies a single spectrum from a 2D Workspace into a new workspace.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the workspace to take as input. </LI>
    <LI> OutputWorkspace - The name under which to store the output workspace.
   </LI>
    <LI> SpectrumIndex - The workspace index number of the spectrum to extract.
   </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 30/06/2009
*/
class DLLExport ExtractSingleSpectrum : public API::DistributedAlgorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "ExtractSingleSpectrum"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Extracts the specified spectrum from a workspace and places it in "
           "a new single-spectrum workspace.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"CropWorkspace", "ExtractSpectra", "PerformIndexOperations"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Transforms\\Splitting";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_EXTRACTSINGLESPECTRUM_H_*/
