// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_SCALE_H_
#define MANTID_ALGORITHMS_SCALE_H_

#include "MantidAPI/DistributedAlgorithm.h"

namespace Mantid {
namespace Algorithms {
/** Scales an input workspace by the given factor, which can be either
   multiplicative or additive.

    Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace. </LI>
    <LI> OutputWorkspace - The name of the output workspace. Can be the same as
   the input one. </LI>
    <LI> Factor          - The value by which to scale the input workspace.
   </LI>
    <LI> Operation       - Whether to multiply (the default) or add by Factor.
   </LI>
    </UL>

    @author Russell Taylor, Tessella plc
    @date 19/03/2010
*/
class DLLExport Scale : public API::DistributedAlgorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "Scale"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Scales an input workspace by the given factor, which can be either "
           "multiplicative or additive.";
  }
  std::map<std::string, std::string> validateInputs() override;

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"ScaleX"}; }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Arithmetic;CorrectionFunctions";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SCALE_H_*/
