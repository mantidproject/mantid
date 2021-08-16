// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidAlgorithms/MultipleScatteringCorrectionDistGraber.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace Algorithms {

using namespace Geometry;

/** MultipleScatteringCorrection : TODO: DESCRIPTION
 */
class MANTID_ALGORITHMS_DLL MultipleScatteringCorrection : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "MultipleScatteringCorrection"; };

  /// Algorithm's version
  int version() const override { return 1; };

  /// Algorithm's category
  const std::string category() const override { return "CorrectionFunctions"; };

  /// Algorithm's summary
  /// TODO: the summary needs to be updated as we add more features
  const std::string summary() const override {
    return "Calculate Multiple Scattering Correction using numerical integration with the assumption of sample only, "
           "elastic scattering only, and isotropic scattering within the sample.";
  };

  /// Algorithm's see also
  const std::vector<std::string> seeAlso() const override {
    return {"CalculateCarpenterSampleCorrection", "CarpenterSampleCorrection", "MayersSampleCorrection"};
  };

protected:
  API::MatrixWorkspace_sptr m_inputWS;  ///< A pointer to the input workspace
  API::MatrixWorkspace_sptr m_outputWS; ///< A pointer to the output workspace
  Kernel::V3D m_beamDirection;          ///< The direction of the beam.
  int64_t m_num_lambda;                 ///< The number of points in wavelength, the rest is interpolated linearly
  double m_elementSize;                 ///< The size of the sample in meters

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;

  void parseInputs();
  void setupOutput();
  void calculateL2Ds(const MultipleScatteringCorrectionDistGraber &distGraber, const IDetector &detector,
                     std::vector<double> &sample_L2Ds) const;
  void pairWiseSum(double &A1, double &A2, const double linearCoefAbs,
                   const MultipleScatteringCorrectionDistGraber &distGraber, const std::vector<double> &L2Ds,
                   const std::vector<double> &L12s, const size_t startIndex, const size_t endIndex) const;

  Kernel::Material m_material;
  double m_sampleLinearCoefTotScatt; ///< The total scattering cross-section in
                                     ///< 1/m for the sample
  int64_t m_xStep;                   ///< The step in bin number between adjacent points for linear interpolation
};

} // namespace Algorithms
} // namespace Mantid
