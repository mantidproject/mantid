// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAlgorithms/Rebin2D.h"
#include "MantidAlgorithms/SofQCommon.h"
#include <list>

namespace Mantid {
namespace Algorithms {

/**
Converts a 2D workspace that has axes of energy transfer against spectrum number
to
one that gives intensity as a function of momentum transfer against energy. This
version
uses proper parallelpiped rebinning to compute the overlap of the various
overlapping
weights

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

@author Martyn Giggg
@date 2011-07-15
 */
class MANTID_ALGORITHMS_DLL SofQWPolygon : public Rebin2D, public API::DeprecatedAlgorithm {
public:
  /// Default constructor
  SofQWPolygon();
  /// Algorithm's name for identification
  const std::string name() const override { return "SofQWPolygon"; }
  const std::string alias() const override { return "SofQW2"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculate the intensity as a function of momentum transfer and "
           "energy.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"SofQW", "SofQWNormalisedPolygon", "Rebin2D"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Inelastic\\SofQW"; }

private:
  /// Initialize the algorithm
  void init() override;
  /// Run the algorithm
  void exec() override;
  /// Init variables cache base on the given workspace
  void initCachedValues(const API::MatrixWorkspace_const_sptr &workspace);
  /// Init the theta index
  void initThetaCache(const API::MatrixWorkspace &workspace);

  SofQCommon m_EmodeProperties;
  /// Output Q axis
  std::vector<double> m_Qout;
  /// Input theta points
  std::vector<double> m_thetaPts;
  /// Theta width
  double m_thetaWidth;
};

} // namespace Algorithms
} // namespace Mantid
