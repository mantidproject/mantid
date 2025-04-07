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
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"
#include <boost/container/small_vector.hpp>
#include <shared_mutex>

namespace Mantid {
namespace API {
class Sample;
}
namespace Geometry {
class Instrument;
}

namespace Algorithms {

// define some simple classes to store 2D datasets instead of using MatrixWorkspace internally
// This couples the algorithm more loosely to Mantid and avoids some complexity in choosing whether
// to call readX, dataX etc
struct DiscusData1D {
  // separate vectors of X and Y rather than vector of pairs to mirror Histogram class and support edges\points
  std::vector<double> X;
  std::vector<double> Y;
  DiscusData1D() {};
  DiscusData1D(std::vector<double> X, std::vector<double> Y) : X(std::move(X)), Y(std::move(Y)) {}
};

class DiscusData2D {
public:
  DiscusData2D() : m_data(std::vector<DiscusData1D>{}), m_specAxis(nullptr) {};
  DiscusData2D(const std::vector<DiscusData1D> &data, const std::shared_ptr<std::vector<double>> &specAxis)
      : m_data(data), m_specAxis(specAxis) {};
  std::unique_ptr<DiscusData2D> createCopy(bool clearY = false);
  size_t getNumberHistograms() { return m_data.size(); }
  DiscusData1D &histogram(const size_t i) { return m_data[i]; }
  std::vector<DiscusData1D> &histograms() { return m_data; }
  const std::vector<double> &getSpecAxisValues();

private:
  std::vector<DiscusData1D> m_data;
  // optional spectrum axis
  std::shared_ptr<std::vector<double>> m_specAxis;
};

struct ComponentWorkspaceMapping {
  Geometry::IObject_const_sptr ComponentPtr;
  std::string_view materialName;
  std::shared_ptr<DiscusData2D> SQ;
  std::shared_ptr<DiscusData2D> logSQ{};
  std::shared_ptr<DiscusData1D> QSQScaleFactor{};
  std::shared_ptr<DiscusData2D> QSQ{};
  std::shared_ptr<DiscusData2D> InvPOfQ{};
  std::shared_ptr<int> scatterCount = std::make_shared<int>(0);
};

/** Object for holding collimator parameteres loaded from instrument parameters file
 */
struct CollimatorInfo {
  double m_innerRadius;
  double m_halfAngularExtent;
  double m_plateHeight;
  Kernel::V3D m_axisVec;
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
  double interpolateFlat(const DiscusData1D &histToInterpolate, double x);
  std::tuple<double, int> sampleQW(const std::shared_ptr<DiscusData2D> &CumulativeProb, double x);
  double interpolateSquareRoot(const DiscusData1D &histToInterpolate, double x);
  double interpolateGaussian(const DiscusData1D &histToInterpolate, double x);
  double Interpolate2D(const ComponentWorkspaceMapping &SQWSMapping, double q, double w);
  void updateTrackDirection(Geometry::Track &track, const double cosT, const double phi);
  void integrateCumulative(const DiscusData1D &h, const double xmin, const double xmax, std::vector<double> &resultX,
                           std::vector<double> &resultY, const bool returnCumulative);
  API::MatrixWorkspace_sptr integrateWS(const API::MatrixWorkspace_sptr &ws);
  void getXMinMax(const Mantid::API::MatrixWorkspace &ws, double &xmin, double &xmax) const;
  void prepareSampleBeamGeometry(const API::MatrixWorkspace_sptr &inputWS);
  const std::shared_ptr<Geometry::CSGObject>
  createCollimatorHexahedronShape(const Kernel::V3D &samplePos, const Mantid::Geometry::DetectorInfo &detectorInfo,
                                  const size_t &histogramIndex);

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
  API::MatrixWorkspace_sptr createOutputWorkspace(const API::MatrixWorkspace &inputWS) const;
  std::tuple<double, double> new_vector(const Kernel::Material &material, double k, bool specialSingleScatterCalc);
  std::tuple<std::vector<double>, std::vector<double>>
  simulatePaths(const int nEvents, const int nScatters, Kernel::PseudoRandomNumberGenerator &rng,
                const ComponentWorkspaceMappings &componentWorkspaces, const double kinc,
                const std::vector<double> &wValues, bool specialSingleScatterCalc,
                const Mantid::Geometry::DetectorInfo &detectorInfo, const size_t &histogramIndex);
  std::tuple<bool, std::vector<double>> scatter(const int nScatters, Kernel::PseudoRandomNumberGenerator &rng,
                                                const ComponentWorkspaceMappings &componentWorkspaces,
                                                const double kinc, const std::vector<double> &wValues,
                                                bool specialSingleScatterCalc,
                                                const Mantid::Geometry::DetectorInfo &detectorInfo,
                                                const size_t &histogramIndex);

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
  void convertToLogWorkspace(const std::shared_ptr<DiscusData2D> &SOfQ);
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
  integrateQSQ(const std::shared_ptr<DiscusData2D> &QSQ, double kinc, const bool returnCumulative);
  double getQSQIntegral(const DiscusData1D &QSQScaleFactor, double k);
  const ComponentWorkspaceMapping *findMatchingComponent(const ComponentWorkspaceMappings &componentWorkspaces,
                                                         const Geometry::IObject *shapeObjectWithScatter);
  void addWorkspaceToDiscus2DData(const Geometry::IObject_const_sptr &shape, const std::string_view &matName,
                                  API::MatrixWorkspace_sptr ws);
  void loadCollimatorInfo();
  double getDoubleParamFromIDF(std::string paramName);
  Kernel::V3D getV3DParamFromIDF(std::string paramName);
  const std::shared_ptr<Geometry::CSGObject> readFromCollimatorCorridorCache(const std::size_t &histogramIndex);
  void writeToCollimatorCorridorCache(const std::size_t &histogramIndex,
                                      const std::shared_ptr<Geometry::CSGObject> &collimatorCorridorCsgObj);
  long long m_callsToInterceptSurface{0};
  long long m_IkCalculations{0};
  std::map<int, int> m_attemptsToGenerateInitialTrack;
  int m_maxScatterPtAttempts{};
  std::shared_ptr<const DiscusData1D> m_sigmaSS; // scattering cross section as a function of k
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
  Mantid::Geometry::Instrument_const_sptr m_instrument;
  std::unique_ptr<CollimatorInfo> m_collimatorInfo;
  std::map<std::size_t, std::shared_ptr<Geometry::CSGObject>> m_collimatorCorridorCache;
  mutable std::shared_mutex m_mutexCorridorCache;
};
} // namespace Algorithms
} // namespace Mantid
