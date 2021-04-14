// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/** EstimateResolutionDiffraction : TODO: DESCRIPTION
 */
class MANTID_ALGORITHMS_DLL EstimateResolutionDiffraction : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override;

  /// function to return any aliases to the algorithm
  const std::string alias() const override;

  /// Summary of algorithms purpose
  const std::string summary() const override;

  /// Algorithm's version for identification overriding a virtual method
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"EstimateDivergence"}; }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override;

private:
  /// Implement abstract Algorithm methods
  void init() override;
  /// Implement abstract Algorithm methods
  void exec() override;

  /// Returns the wavelength from either the property or the input workspace
  double getWavelength();

  /// Process input properties for algorithm
  void processAlgProperties();

  ///
  void retrieveInstrumentParameters();

  /// Calculate detector resolution
  void estimateDetectorResolution();

  /// Input workspace
  API::MatrixWorkspace_sptr m_inputWS;
  /// Workspace with custom divergence term
  API::MatrixWorkspace_sptr m_divergenceWS;

  /// workspace holding the term for just the time-of-flight portion of the
  /// resolution
  API::MatrixWorkspace_sptr m_resTof;
  /// workspace holding the term for just the flight path portion of the
  /// resolution
  API::MatrixWorkspace_sptr m_resPathLength;
  /// workspace holding the term for just the angular/solid angle portion of the
  /// resolution
  API::MatrixWorkspace_sptr m_resAngle;

  /// Output workspace
  API::MatrixWorkspace_sptr m_outputWS;

  /// Centre neutron velocity
  double m_centreVelocity = 0.0;

  /// Delta T
  double m_deltaT = 0.0;
};

} // namespace Algorithms
} // namespace Mantid
