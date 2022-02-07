// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/** Makes a histogram workspace a distribution. i.e. divides by the bin width

    Required Properties:
    <UL>
    <LI> Workspace - The name of the Workspace to convert.</LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 17/11/2008
*/
class MANTID_ALGORITHMS_DLL ConvertToDistribution : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "ConvertToDistribution"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Makes a histogram workspace a distribution i.e. divides by the bin "
           "width.";
  }
  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"ConvertFromDistribution"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Transforms\\Distribution"; }

protected:
  /// Validate inputs
  std::map<std::string, std::string> validateInputs() override;

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
