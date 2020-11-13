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
#include "MantidAlgorithms/DllConfig.h"
#include "MantidAlgorithms/SampleCorrections/SparseWorkspace.h"

namespace Mantid {
namespace API {
class Sample;
}
namespace Geometry {
class Instrument;
}

namespace Algorithms {

/** Calculates a multiple scattering correction
* Based on Fortran code provided by Spencer Howells

  @author Danny Hindson
  @date 2020-11-10
*/
class MANTID_ALGORITHMS_DLL MuscatElastic : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "MuscatElastic"; }
  /// Algorithm's version
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"MayersSampleCorrection", "CarpenterSampleCorrection",
            "PearlMCAbsorption", "VesuvioCalculateMS"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "CorrectionFunctions\\AbsorptionCorrections";
  }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculates multiple scattering correction using a Monte Carlo "
           "method";
  }

protected:
  virtual std::shared_ptr<SparseWorkspace>
  createSparseWorkspace(const API::MatrixWorkspace &modelWS,
                        const size_t wavelengthPoints, const size_t rows,
                        const size_t columns);

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
  API::MatrixWorkspace_uptr
  createOutputWorkspace(const API::MatrixWorkspace &inputWS) const;
  std::tuple<double, double> new_vector(double absorbXsection,
                                        double numberDensity,
                                        double totalScatterXsection);
  double interpolateSigmaS(const API::MatrixWorkspace_sptr sigmaS, double kinc);
  void scatter(bool doMultipleScattering, int nScatters, double absorbXsection);
  Kernel::V3D start_point();
};
} // namespace Algorithms
} // namespace Mantid
