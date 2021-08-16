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
#include "MantidAlgorithms/InterpolationOption.h"
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
* Based on Muscat Fortran code provided by Spencer Howells

  @author Danny Hindson
  @date 2020-11-10
*/
class MANTID_ALGORITHMS_DLL CalculateMultipleScattering : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "CalculateMultipleScattering"; }
  /// Algorithm's version
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"MayersSampleCorrection", "CarpenterSampleCorrection", "VesuvioCalculateMS"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "CorrectionFunctions"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculates a multiple scattering correction using a Monte Carlo method";
  }
  const std::string alias() const override { return "Muscat"; }

protected:
  virtual std::shared_ptr<SparseWorkspace> createSparseWorkspace(const API::MatrixWorkspace &modelWS,
                                                                 const size_t wavelengthPoints, const size_t rows,
                                                                 const size_t columns);
  virtual std::unique_ptr<InterpolationOption> createInterpolateOption();
  double interpolateGaussian(const HistogramData::Histogram &histToInterpolate, double x);
  void updateTrackDirection(Geometry::Track &track, const double cosT, const double phi);

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
  API::MatrixWorkspace_sptr createOutputWorkspace(const API::MatrixWorkspace &inputWS) const;
  double new_vector(const API::MatrixWorkspace_sptr sigmaSSWS, const Kernel::Material &material, double kinc,
                    bool specialSingleScatterCalc);
  double simulatePaths(const int nEvents, const int nScatters, const API::Sample &sample,
                       const Geometry::Instrument &instrument, Kernel::PseudoRandomNumberGenerator &rng,
                       const API::MatrixWorkspace_sptr sigmaSSWS, const HistogramData::Histogram &SOfQ,
                       const double kinc, Kernel::V3D detPos, bool specialSingleScatterCalc);
  std::tuple<bool, double, double> scatter(const int nScatters, const API::Sample &sample,
                                           const Geometry::Instrument &instrument, Kernel::V3D sourcePos,
                                           Kernel::PseudoRandomNumberGenerator &rng, const double sigma_total,
                                           double scatteringXSection, const HistogramData::Histogram &SOfQ,
                                           const double kinc, Kernel::V3D detPos, bool specialSingleScatterCalc);
  Geometry::Track start_point(const Geometry::IObject &shape, std::shared_ptr<const Geometry::ReferenceFrame>,
                              Kernel::V3D sourcePos, Kernel::PseudoRandomNumberGenerator &rng);
  Geometry::Track generateInitialTrack(const Geometry::IObject &shape,
                                       std::shared_ptr<const Geometry::ReferenceFrame> frame,
                                       const Kernel::V3D &sourcePos, Kernel::PseudoRandomNumberGenerator &rng);
  void inc_xyz(Geometry::Track &track, double vl);
  void updateWeightAndPosition(Geometry::Track &track, double &weight, const double vmfp, const double sigma_total,
                               Kernel::PseudoRandomNumberGenerator &rng);
  void q_dir(Geometry::Track &track, const HistogramData::Histogram &SOfQ, const double kinc,
             const double scatteringXSection, Kernel::PseudoRandomNumberGenerator &rng, double &QSS, double &weight);
  void interpolateFromSparse(API::MatrixWorkspace &targetWS, const SparseWorkspace &sparseWS,
                             const Mantid::Algorithms::InterpolationOption &interpOpt);
  void correctForWorkspaceNameClash(std::string &wsName);
  void setWorkspaceName(const API::MatrixWorkspace_sptr &ws, std::string wsName);
  long long m_callsToInterceptSurface{0};
  std::map<int, int> m_attemptsToGenerateInitialTrack;
};
} // namespace Algorithms
} // namespace Mantid
