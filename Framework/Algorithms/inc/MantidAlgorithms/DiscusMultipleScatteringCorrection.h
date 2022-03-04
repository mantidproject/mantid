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
class MANTID_ALGORITHMS_DLL DiscusMultipleScatteringCorrection : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "DiscusMultipleScatteringCorrection"; }
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
  double interpolateFlat(const API::ISpectrum &histToInterpolate, double x);
  std::tuple<double, int> sampleQW(const API::MatrixWorkspace_sptr &CumulativeProb, double x);
  double interpolateSquareRoot(const API::ISpectrum &histToInterpolate, double x);
  double interpolateGaussian(const API::ISpectrum &histToInterpolate, double x);
  double Interpolate2D(API::MatrixWorkspace_sptr SOfQ, double w, double q);
  void updateTrackDirection(Geometry::Track &track, const double cosT, const double phi);
  void integrateCumulative(const Mantid::HistogramData::Histogram &h, double xmin, double xmax,
                           std::vector<double> &resultX, std::vector<double> &resultY);

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
  API::MatrixWorkspace_sptr createOutputWorkspace(const API::MatrixWorkspace &inputWS) const;
  std::tuple<double, double> new_vector(const Kernel::Material &material, double kinc, bool specialSingleScatterCalc);
  std::vector<double> simulatePaths(const int nEvents, const int nScatters, Kernel::PseudoRandomNumberGenerator &rng,
                                    API::MatrixWorkspace_sptr &invPOfQ, const double kinc,
                                    const std::vector<double> &wValues, const Kernel::V3D &detPos,
                                    bool specialSingleScatterCalc);
  std::tuple<bool, std::vector<double>, double> scatter(const int nScatters, Kernel::PseudoRandomNumberGenerator &rng,
                                                        const API::MatrixWorkspace_sptr &invPOfQ, const double kinc,
                                                        const std::vector<double> &wValues, const Kernel::V3D &detPos,
                                                        bool specialSingleScatterCalc);
  Geometry::Track start_point(Kernel::PseudoRandomNumberGenerator &rng);
  Geometry::Track generateInitialTrack(Kernel::PseudoRandomNumberGenerator &rng);
  void inc_xyz(Geometry::Track &track, double vl);
  void updateWeightAndPosition(Geometry::Track &track, double &weight, const double vmfp, const double sigma_total,
                               Kernel::PseudoRandomNumberGenerator &rng);
  void q_dir(Geometry::Track &track, const API::MatrixWorkspace_sptr &invPOfQ, double &k,
             const double scatteringXSection, Kernel::PseudoRandomNumberGenerator &rng, double &QSS, double &weight);
  void interpolateFromSparse(API::MatrixWorkspace &targetWS, const SparseWorkspace &sparseWS,
                             const Mantid::Algorithms::InterpolationOption &interpOpt);
  void correctForWorkspaceNameClash(std::string &wsName);
  void setWorkspaceName(const API::MatrixWorkspace_sptr &ws, std::string wsName);
  API::MatrixWorkspace_sptr createInvPOfQ(size_t expectedSize);
  void prepareCumulativeProbForQ(double kinc, const API::MatrixWorkspace_sptr &PInvOfQ);
  void getXMinMax(const Mantid::API::MatrixWorkspace &ws, double &xmin, double &xmax) const;
  API::MatrixWorkspace_uptr prepareQSQ(double kinc);
  double getKf(const API::MatrixWorkspace_sptr &SOfQ, const int iW, const double kinc);
  std::tuple<double, int> sampleKW(const API::MatrixWorkspace_sptr &SOfQ, Kernel::PseudoRandomNumberGenerator &rng,
                                   const double kinc);
  void convertWsToPoints(API::MatrixWorkspace_sptr &ws);
  std::tuple<double, double> getKinematicRange(double kf, double ki);
  long long m_callsToInterceptSurface{0};
  std::map<int, int> m_attemptsToGenerateInitialTrack;
  int m_maxScatterPtAttempts;
  std::shared_ptr<const DataObjects::Histogram1D> m_sigmaSS; // scattering cross section as a function of k
  API::MatrixWorkspace_sptr m_SQWS;
  API::MatrixWorkspace_sptr m_QSQWS;
  API::MatrixWorkspace_sptr m_logSQ;
  Geometry::IObject_const_sptr m_sampleShape;
  bool m_importanceSampling;
  Kernel::DeltaEMode::Type m_EMode;
  bool m_simulateEnergiesIndependently;
  Kernel::V3D m_sourcePos;
  std::shared_ptr<const Geometry::ReferenceFrame> m_refframe;
};
} // namespace Algorithms
} // namespace Mantid
