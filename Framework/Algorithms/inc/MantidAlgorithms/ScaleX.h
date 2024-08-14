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
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"

#include <boost/function.hpp>

namespace Mantid {
namespace Algorithms {
/**Takes a workspace and adjusts all the time bin values by the same
multiplicative factor.

Required Properties:
<UL>
<LI> InputWorkspace - The name of the Workspace to take as input </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the result
</LI>
<LI> Factor - The scaling factor to multiply the time bins by</LI>
</UL>

@author
@date 6/23/2011
*/
class MANTID_ALGORITHMS_DLL ScaleX final : public API::Algorithm {
public:
  /// Default constructor
  ScaleX();

  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "ScaleX"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Scales the X-axis of an input workspace by the given factor, which "
           "can be either multiplicative or additive.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"ChangeBinOffset", "Scale"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Arithmetic;CorrectionFunctions"; }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  /// Execute algorithm for EventWorkspaces
  void execEvent();

  /// Create output workspace
  API::MatrixWorkspace_sptr createOutputWS(const API::MatrixWorkspace_sptr &input);
  /// Get the scale factor for the given spectrum
  double getScaleFactor(const API::MatrixWorkspace_const_sptr &inputWS, const Mantid::API::SpectrumInfo &spectrumInfo,
                        const size_t index);

  /// The progress reporting object
  std::unique_ptr<API::Progress> m_progress;

  /// Scaling factor
  double m_algFactor;
  /// instrument parameter name
  std::string m_parname;
  /// Flag whether we are combining input parameters
  bool m_combine;
  /// Function defining request operation
  boost::function<double(double, double)> m_binOp;
  /// Start workspace index
  int m_wi_min;
  /// Stop workspace index
  int m_wi_max;
};

} // namespace Algorithms
} // namespace Mantid
