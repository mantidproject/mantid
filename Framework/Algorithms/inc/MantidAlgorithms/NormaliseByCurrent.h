// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DistributedAlgorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include <memory>
namespace Mantid {
namespace API {
// Forward declare
class MatrixWorkspace;
} // namespace API
namespace Algorithms {
/** Normalises a workspace according to the good proton charge figure taken from
   the
    raw file (and stored in the Sample object). Every data point is divided by
   that number.
    Note that units are not fully dealt with at the moment - the output will
   have identical
    units to the input workspace (i.e. will not show "/ uA.hour").

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The names of the input workspace. </LI>
    <LI> OutputWorkspace - The name of the output workspace. </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 25/08/2008
*/
class MANTID_ALGORITHMS_DLL NormaliseByCurrent : public API::DistributedAlgorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "NormaliseByCurrent"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Normalises a workspace by the proton charge."; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"Divide"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "CorrectionFunctions\\NormalisationCorrections"; }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  // Extract the charge value from the logs.
  double extractCharge(const std::shared_ptr<Mantid::API::MatrixWorkspace> &inputWS, const bool integratePCharge) const;
};

} // namespace Algorithms
} // namespace Mantid
