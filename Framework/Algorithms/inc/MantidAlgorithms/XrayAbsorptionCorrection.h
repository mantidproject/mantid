// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/InterpolationOption.h"
#include "MantidAlgorithms/SampleCorrections/IBeamProfile.h"
#include "MantidAlgorithms/SampleCorrections/MCAbsorptionStrategy.h"
#include "MantidAlgorithms/SampleCorrections/MCInteractionVolume.h"
#include "MantidAlgorithms/SampleCorrections/SparseWorkspace.h"

namespace Mantid {
namespace API {
class Sample;
}
namespace Geometry {
class Instrument;
}

namespace Algorithms {
class DetectorGridDefinition;
class MCInteractionVolume;

/**
 *Calculates attenuation of xrays due to absorption in a sample.
 */
class MANTID_ALGORITHMS_DLL XrayAbsorptionCorrection : public API::Algorithm {
public:
  ~XrayAbsorptionCorrection() override = default;
  /// Algorithm's name
  const std::string name() const override { return "XrayAbsorptionCorrection"; }
  /// Algorithm's version
  int version() const override { return 1; }
  /// Algorithm's category for identification
  const std::string category() const override { return "CorrectionFunctions\\AbsorptionCorrections"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculates attenuation of xrays produced by negative muons due"
           " to absorption in a sample for elemental analysis ";
  }
  const std::vector<std::string> seeAlso() const override { return {"SetSample", "SetSampleMaterial"}; }

protected:
  double degreesToRadians(double degrees);

  Kernel::V3D calculateDetectorPos(double const detectorAngle, double detectorDistance);

  std::vector<Kernel::V3D> calculateMuonPos(API::MatrixWorkspace_sptr &muonProfile,
                                            const API::MatrixWorkspace_sptr &inputWS, double detectorDistance);

  std::vector<double> normaliseMuonIntensity(MantidVec muonIntensity);

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
};
} // namespace Algorithms
} // namespace Mantid
