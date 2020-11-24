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
#include "MantidGeometry/IComponent.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"

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
  double interpolateLogQuadratic(
      const API::MatrixWorkspace_sptr workspaceToInterpolate, double x);
  double simulateEvents(const int nEvents, const size_t nScatters,
                        const double absorbXsection, const API::Sample &sample,
                        const Geometry::Instrument &instrument,
                        Kernel::PseudoRandomNumberGenerator &rng,
                        const double vmfp, const double sigma_total,
                        double scatteringXSection,
                        const API::MatrixWorkspace_sptr SOfQ, const double kinc,
                        Kernel::V3D detPos);
  std::tuple<bool, double>
  scatter(const size_t nScatters, const double absorbXsection,
          const API::Sample &sample, const Geometry::Instrument &instrument,
          Kernel::V3D sourcePos, Kernel::PseudoRandomNumberGenerator &rng,
          const double vmfp, const double sigma_total,
          double scatteringXSection, const API::MatrixWorkspace_sptr SOfQ,
          const double kinc, Kernel::V3D detPos);
  Geometry::Track start_point(const API::Sample &sample,
                              std::shared_ptr<const Geometry::ReferenceFrame>,
                              Kernel::V3D sourcePos,
                              Kernel::PseudoRandomNumberGenerator &rng);
  Geometry::Track
  generateInitialTrack(const API::Sample &sample,
                       std::shared_ptr<const Geometry::ReferenceFrame> frame,
                       const Kernel::V3D sourcePos,
                       Kernel::PseudoRandomNumberGenerator &rng);
  void inc_xyz(Geometry::Track &track, double vl);
  void updateWeightAndPosition(Geometry::Track &track, double &weight,
                               const double vmfp, const double sigma_total,
                               Kernel::PseudoRandomNumberGenerator &rng);
  void q_dir(Geometry::Track track, const API::MatrixWorkspace_sptr SOfQ,
             const double kinc, double scatteringXSection,
             Kernel::PseudoRandomNumberGenerator &rng, double &QSS,
             double &weight);
  int m_callsToInterceptSurface;
};
} // namespace Algorithms
} // namespace Mantid
