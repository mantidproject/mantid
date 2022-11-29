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
#include "MantidAlgorithms/SampleCorrections/IBeamProfile.h"
#include "MantidAlgorithms/SampleCorrections/SparseWorkspace.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"

#include <boost/container/small_vector.hpp>

namespace Mantid {
namespace API {
class Sample;
}
namespace Geometry {
class Instrument;
}

namespace Algorithms {

struct ComponentWorkspaceMapping {
  Geometry::IObject_const_sptr ComponentPtr;
  std::string_view materialName;
  API::MatrixWorkspace_sptr SQ;
  API::MatrixWorkspace_sptr logSQ{};
  std::shared_ptr<DataObjects::Histogram1D> QSQScaleFactor{};
  API::MatrixWorkspace_sptr QSQ{};
  API::MatrixWorkspace_sptr InvPOfQ{};
  std::shared_ptr<int> scatterCount = std::make_shared<int>(0);
};

/** Calculates a multiple scattering correction
* Based on Muscat Fortran code provided by Spencer Howells

  @author Danny Hindson
  @date 2020-11-10
*/
class MANTID_ALGORITHMS_DLL DiscusMultipleScatteringCorrection : public API::Algorithm {
public:
  // use small_vector to avoid performance hit from heap allocation of std::vector. Use size 5 in line with Track.h
  using ComponentWorkspaceMappings = boost::container::small_vector<ComponentWorkspaceMapping, 5>;
  virtual ~DiscusMultipleScatteringCorrection() = default;
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
  bool checkGroups() override { return false; }

protected:
  virtual std::shared_ptr<SparseWorkspace> createSparseWorkspace(const API::MatrixWorkspace &modelWS,
                                                                 const size_t nXPoints, const size_t rows,
                                                                 const size_t columns);
  virtual std::unique_ptr<InterpolationOption> createInterpolateOption();
  double interpolateFlat(const API::ISpectrum &histToInterpolate, double x);
  std::tuple<double, int> sampleQW(const API::MatrixWorkspace_sptr &CumulativeProb, double x);
  double interpolateSquareRoot(const API::ISpectrum &histToInterpolate, double x);
  double interpolateGaussian(const API::ISpectrum &histToInterpolate, double x);
  double Interpolate2D(const ComponentWorkspaceMapping &SQWSMapping, double q, double w);
  void updateTrackDirection(Geometry::Track &track, const double cosT, const double phi);
  void integrateCumulative(const API::ISpectrum &h, const double xmin, const double xmax, std::vector<double> &resultX,
                           std::vector<double> &resultY, const bool returnCumulative);
  API::MatrixWorkspace_sptr integrateWS(const API::MatrixWorkspace_sptr &ws);
  void getXMinMax(const Mantid::API::MatrixWorkspace &ws, double &xmin, double &xmax) const;
  void prepareSampleBeamGeometry(const API::MatrixWorkspace_sptr &inputWS);

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
  API::MatrixWorkspace_sptr createOutputWorkspace(const API::MatrixWorkspace &inputWS) const;
  std::tuple<double, double> new_vector(const Kernel::Material &material, double k, bool specialSingleScatterCalc);
  std::vector<double> simulatePaths(const int nEvents, const int nScatters, Kernel::PseudoRandomNumberGenerator &rng,
                                    ComponentWorkspaceMappings &componentWorkspaces, const double kinc,
                                    const std::vector<double> &wValues, const Kernel::V3D &detPos,
                                    bool specialSingleScatterCalc);
  std::tuple<bool, std::vector<double>> scatter(const int nScatters, Kernel::PseudoRandomNumberGenerator &rng,
                                                const ComponentWorkspaceMappings &componentWorkspaces,
                                                const double kinc, const std::vector<double> &wValues,
                                                const Kernel::V3D &detPos, bool specialSingleScatterCalc);
  Geometry::Track start_point(Kernel::PseudoRandomNumberGenerator &rng);
  Geometry::Track generateInitialTrack(Kernel::PseudoRandomNumberGenerator &rng);
  void inc_xyz(Geometry::Track &track, double vl);
  const Geometry::IObject *updateWeightAndPosition(Geometry::Track &track, double &weight, const double k,
                                                   Kernel::PseudoRandomNumberGenerator &rng,
                                                   bool specialSingleScatterCalc,
                                                   const ComponentWorkspaceMappings &componentWorkspaces);
  bool q_dir(Geometry::Track &track, const Geometry::IObject *shapePtr, const ComponentWorkspaceMappings &invPOfQs,
             double &k, const double scatteringXSection, Kernel::PseudoRandomNumberGenerator &rng, double &weight);
  void interpolateFromSparse(API::MatrixWorkspace &targetWS, const SparseWorkspace &sparseWS,
                             const Mantid::Algorithms::InterpolationOption &interpOpt);
  void correctForWorkspaceNameClash(std::string &wsName);
  void setWorkspaceName(const API::MatrixWorkspace_sptr &ws, std::string wsName);
  void createInvPOfQWorkspaces(ComponentWorkspaceMappings &matWSs, size_t nhists);
  void convertToLogWorkspace(const API::MatrixWorkspace_sptr &SOfQ);
  void calculateQSQIntegralAsFunctionOfK(ComponentWorkspaceMappings &matWSs, const std::vector<double> &specialKs);
  void prepareCumulativeProbForQ(double kinc, const ComponentWorkspaceMappings &PInvOfQs);
  void prepareQSQ(double kinc);
  double getKf(const double deltaE, const double kinc);
  std::tuple<double, double, int, double> sampleQWUniform(const std::vector<double> &wValues,
                                                          Kernel::PseudoRandomNumberGenerator &rng, const double kinc);
  void prepareStructureFactors();
  void convertWsBothAxesToPoints(API::MatrixWorkspace_sptr &ws);
  std::tuple<double, double> getKinematicRange(double kf, double ki);
  std::vector<std::tuple<double, int, double>> generateInputKOutputWList(const double efixed,
                                                                         const std::vector<double> &xPoints);
  std::tuple<std::vector<double>, std::vector<double>, std::vector<double>>
  integrateQSQ(const API::MatrixWorkspace_sptr &QSQ, double kinc, const bool returnCumulative);
  double getQSQIntegral(const API::ISpectrum &QSQScaleFactor, double k);
  const ComponentWorkspaceMapping *findMatchingComponent(const ComponentWorkspaceMappings &componentWorkspaces,
                                                         const Geometry::IObject *shapeObjectWithScatter);
  long long m_callsToInterceptSurface{0};
  long long m_IkCalculations{0};
  std::map<int, int> m_attemptsToGenerateInitialTrack;
  int m_maxScatterPtAttempts{};
  std::shared_ptr<const DataObjects::Histogram1D> m_sigmaSS; // scattering cross section as a function of k
  // vectors of S(Q,w) and derived quantities. One entry for sample and each environment component
  ComponentWorkspaceMappings m_SQWSs;
  Geometry::IObject_const_sptr m_sampleShape;
  bool m_importanceSampling{};
  Kernel::DeltaEMode::Type m_EMode{Kernel::DeltaEMode::Undefined};
  bool m_simulateEnergiesIndependently{};
  Kernel::V3D m_sourcePos;
  std::shared_ptr<const Geometry::ReferenceFrame> m_refframe;
  const Geometry::SampleEnvironment *m_env{nullptr};
  bool m_NormalizeSQ{};
  Geometry::BoundingBox m_activeRegion;
  std::unique_ptr<IBeamProfile> m_beamProfile;
};
} // namespace Algorithms
} // namespace Mantid
