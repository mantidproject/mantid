// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidAlgorithms/SofQCommon.h"

namespace Mantid {
namespace Algorithms {
/** Converts a 2D workspace that has axes of energy transfer against spectrum
number to
one that gives intensity as a function of momentum transfer against energy.

Required Properties:
<UL>
<LI> InputWorkspace  - Reduced data in units of energy transfer. Must have
common bins. </LI>
<LI> OutputWorkspace - The name to use for the q-w workspace. </LI>
<LI> QAxisBinning    - The bin parameters to use for the q axis. </LI>
<LI> Emode           - The energy mode (direct or indirect geometry). </LI>
<LI> Efixed          - Value of fixed energy: EI (emode=1) or EF (emode=2)
(meV). </LI>
</UL>

@author Russell Taylor, Tessella plc
@date 24/02/2010
*/
class MANTID_ALGORITHMS_DLL SofQWCentre final : public API::Algorithm, public API::DeprecatedAlgorithm {
public:
  /// Default constructor
  SofQWCentre();
  /// Algorithm's name
  const std::string name() const override { return "SofQWCentre"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Converts a 2D workspace that has axes in *units* of **DeltaE** "
           "(energy transfer) against spectrum number to one "
           "that gives intensity as a function of **DeltaE** against "
           "**momentum transfer** ";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"SofQW", "Rebin2D"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Inelastic\\SofQW"; }

private:
  /// Convert the workspace to a distribution
  static void makeDistribution(API::MatrixWorkspace &outputWS, const std::vector<double> &qAxis);
  /// Initialization code
  void init() override;
  /// Execution code
  void exec() override;

  SofQCommon m_EmodeProperties;
};

} // namespace Algorithms
} // namespace Mantid
