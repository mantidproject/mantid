// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CalculateMultipleScattering.h"
#include "MantidAPI/EqualBinSizesValidator.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/MersenneTwister.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::DataObjects::Workspace2D;

namespace {
constexpr int DEFAULT_NPATHS = 1000;
constexpr int DEFAULT_SEED = 123456789;
constexpr int DEFAULT_NSCATTERINGS = 2;
constexpr int DEFAULT_LATITUDINAL_DETS = 5;
constexpr int DEFAULT_LONGITUDINAL_DETS = 10;
} // namespace

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalculateMultipleScattering)

/**
 * Initialize the algorithm
 */
void CalculateMultipleScattering::init() {
  // The input workspace must have an instrument and units of wavelength
  auto wsValidator = std::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<InstrumentValidator>();

  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, wsValidator),
                  "The name of the input workspace.  The input workspace must "
                  "have X units of wavelength. This is used to supply the detector "
                  "positions and the x axis range to calculate corrections for");

  auto wsQValidator = std::make_shared<CompositeValidator>();
  wsQValidator->add<WorkspaceUnitValidator>("MomentumTransfer");
  wsQValidator->add<EqualBinSizesValidator>(1.0E-07);

  declareProperty(std::make_unique<WorkspaceProperty<>>("SofqWorkspace", "", Direction::Input, wsQValidator),
                  "The name of the workspace containing S'(q).  The input "
                  "workspace must contain a single spectrum and "
                  "have X units of momentum transfer.");
  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>("OutputWorkspace", "", Direction::Output),
                  "Name for the WorkspaceGroup that will be created. Each workspace in the "
                  "group contains a calculated weight for a particular number of "
                  "scattering events. The number of scattering events varies from 1 up to "
                  "the number supplied in the NumberOfScatterings parameter. The group "
                  "will also include an additional workspace for a calculation with a "
                  "single scattering event where the absorption post scattering has been "
                  "set to zero");
  declareProperty(std::make_unique<WorkspaceProperty<>>("ScatteringCrossSection", "", Direction::Input,
                                                        PropertyMode::Optional, wsQValidator),
                  "A workspace containing the scattering cross section as a function of "
                  "k.");

  auto positiveInt = std::make_shared<Kernel::BoundedValidator<int>>();
  positiveInt->setLower(1);
  declareProperty("NumberOfWavelengthPoints", EMPTY_INT(), positiveInt,
                  "The number of wavelength points for which a simulation is "
                  "attempted if ResimulateTracksForDifferentWavelengths=true");

  std::vector<std::string> propOptions{"Elastic", "Direct", "Indirect"};
  declareProperty("EMode", "Elastic", std::make_shared<Kernel::StringListValidator>(propOptions),
                  "The energy mode (default: Elastic)");

  declareProperty("NeutronPathsSingle", DEFAULT_NPATHS, positiveInt,
                  "The number of \"neutron\" paths to generate for single scattering");
  declareProperty("NeutronPathsMultiple", DEFAULT_NPATHS, positiveInt,
                  "The number of \"neutron\" paths to generate for multiple scattering");
  declareProperty("SeedValue", DEFAULT_SEED, positiveInt, "Seed the random number generator with this value");
  auto nScatteringsValidator = std::make_shared<Kernel::BoundedValidator<int>>();
  nScatteringsValidator->setLower(1);
  nScatteringsValidator->setUpper(5);
  declareProperty("NumberScatterings", DEFAULT_NSCATTERINGS, nScatteringsValidator, "Number of scatterings");

  auto interpolateOpt = createInterpolateOption();
  declareProperty(interpolateOpt->property(), interpolateOpt->propertyDoc());
  declareProperty("SparseInstrument", false,
                  "Enable simulation on special "
                  "instrument with a sparse grid of "
                  "detectors interpolating the "
                  "results to the real instrument.");
  auto threeOrMore = std::make_shared<Kernel::BoundedValidator<int>>();
  threeOrMore->setLower(3);
  declareProperty("NumberOfDetectorRows", DEFAULT_LATITUDINAL_DETS, threeOrMore,
                  "Number of detector rows in the detector grid of the sparse instrument.");
  setPropertySettings("NumberOfDetectorRows",
                      std::make_unique<EnabledWhenProperty>("SparseInstrument", ePropertyCriterion::IS_NOT_DEFAULT));
  auto twoOrMore = std::make_shared<Kernel::BoundedValidator<int>>();
  twoOrMore->setLower(2);
  declareProperty("NumberOfDetectorColumns", DEFAULT_LONGITUDINAL_DETS, twoOrMore,
                  "Number of detector columns in the detector grid "
                  "of the sparse instrument.");
  setPropertySettings("NumberOfDetectorColumns",
                      std::make_unique<EnabledWhenProperty>("SparseInstrument", ePropertyCriterion::IS_NOT_DEFAULT));
}

/**
 * Validate the input properties.
 * @return a map where keys are property names and values the found issues
 */
std::map<std::string, std::string> CalculateMultipleScattering::validateInputs() {
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  std::map<std::string, std::string> issues;
  Geometry::IComponent_const_sptr sample = inputWS->getInstrument()->getSample();
  if (!sample) {
    issues["InputWorkspace"] = "Input workspace does not have a Sample";
  } else {
    if (inputWS->sample().hasEnvironment()) {
      issues["InputWorkspace"] = "Sample must not have a sample environment";
    }

    if (inputWS->sample().getMaterial().numberDensity() == 0) {
      issues["InputWorkspace"] = "Sample must have a material set up with a non-zero number density";
    }
  }
  MatrixWorkspace_sptr SQWS = getProperty("SofqWorkspace");
  if (SQWS->getNumberHistograms() != 1) {
    issues["SofqWorkspace"] = "S(Q) workspace must contain a single spectrum";
  }
  auto y = SQWS->y(0);
  if (std::any_of(y.cbegin(), y.cend(), [](const auto yval) { return (yval <= 0); })) {
    issues["SofqWorkspace"] = "S(Q) workspace must have all y > 0";
  }

  const std::string emodeStr = getProperty("EMode");
  auto emode = Kernel::DeltaEMode::fromString(emodeStr);
  if (emode != Kernel::DeltaEMode::Elastic) {
    issues["EMode"] = "Only elastic mode is supported at the moment";
  }

  const int nlambda = getProperty("NumberOfWavelengthPoints");
  if (!isEmpty(nlambda)) {
    InterpolationOption interpOpt;
    const std::string interpValue = getPropertyValue("Interpolation");
    interpOpt.set(interpValue, false, false);
    const auto nlambdaIssue = interpOpt.validateInputSize(nlambda);
    if (!nlambdaIssue.empty()) {
      issues["NumberOfWavelengthPoints"] = nlambdaIssue;
    }
  }

  return issues;
}

/**
 * Execution code
 */
void CalculateMultipleScattering::exec() {
  g_log.warning("CalculateMultipleScattering is in the beta stage of development. Its name, properties and behaviour "
                "may change without warning.");
  if (!getAlwaysStoreInADS())
    throw std::runtime_error("This algorithm explicitly stores named output workspaces in the ADS so must be run with "
                             "AlwaysStoreInADS set to true");
  const MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr SQWS = getProperty("SofqWorkspace");
  SQWS = SQWS->clone();
  // avoid repeated conversion of bin edges to points inside loop by converting to point data
  if (SQWS->isHistogramData()) {
    auto pointDataAlgorithm = this->createChildAlgorithm("ConvertToPointData");
    pointDataAlgorithm->initialize();
    pointDataAlgorithm->setProperty("InputWorkspace", SQWS);
    pointDataAlgorithm->setProperty("OutputWorkspace", "_");
    pointDataAlgorithm->execute();
    SQWS = pointDataAlgorithm->getProperty("OutputWorkspace");
  }
  // take log of S(Q) and store it this way
  auto &y = SQWS->mutableY(0);
  std::transform(y.begin(), y.end(), y.begin(), static_cast<double (*)(double)>(std::log));

  MatrixWorkspace_sptr sigmaSSWS = getProperty("ScatteringCrossSection");
  if (sigmaSSWS) {
    sigmaSSWS = sigmaSSWS->clone();
    // take log of sigmaSSWS and store it this way
    auto &y = sigmaSSWS->mutableY(0);
    std::transform(y.begin(), y.end(), y.begin(), static_cast<double (*)(double)>(std::log));
  }
  auto SQHist = SQWS->histogram(0);
  const auto inputNbins = static_cast<int>(inputWS->blocksize());
  int nlambda = getProperty("NumberOfWavelengthPoints");
  if (isEmpty(nlambda) || nlambda > inputNbins) {
    if (!isEmpty(nlambda)) {
      g_log.warning() << "The requested number of wavelength points is larger "
                         "than the spectra size. "
                         "Defaulting to spectra size.\n";
    }
    nlambda = inputNbins;
  }

  const bool useSparseInstrument = getProperty("SparseInstrument");
  SparseWorkspace_sptr sparseWS;
  if (useSparseInstrument) {
    const int latitudinalDets = getProperty("NumberOfDetectorRows");
    const int longitudinalDets = getProperty("NumberOfDetectorColumns");
    sparseWS = createSparseWorkspace(*inputWS, nlambda, latitudinalDets, longitudinalDets);
  }
  const int nScatters = getProperty("NumberScatterings");
  std::vector<MatrixWorkspace_sptr> simulationWSs;
  std::vector<MatrixWorkspace_sptr> outputWSs;

  auto noAbsOutputWS = createOutputWorkspace(*inputWS);
  auto noAbsSimulationWS = useSparseInstrument ? sparseWS->clone() : noAbsOutputWS;
  for (int i = 0; i < nScatters; i++) {
    auto outputWS = createOutputWorkspace(*inputWS);
    MatrixWorkspace_sptr simulationWS = useSparseInstrument ? sparseWS->clone() : outputWS;
    simulationWSs.push_back(simulationWS);
    outputWSs.push_back(outputWS);
  }
  const MatrixWorkspace &instrumentWS = useSparseInstrument ? *sparseWS : *inputWS;

  auto instrument = inputWS->getInstrument();
  const auto nhists = useSparseInstrument ? static_cast<int64_t>(sparseWS->getNumberHistograms())
                                          : static_cast<int64_t>(inputWS->getNumberHistograms());

  const auto &sample = inputWS->sample();
  // generate the bounding box before the multithreaded section
  sample.getShape().getBoundingBox();

  const int nSingleScatterEvents = getProperty("NeutronPathsSingle");
  const int nMultiScatterEvents = getProperty("NeutronPathsMultiple");

  const int seed = getProperty("SeedValue");

  InterpolationOption interpolateOpt;
  interpolateOpt.set(getPropertyValue("Interpolation"), false, true);

  Progress prog(this, 0.0, 1.0, nhists * nlambda);
  prog.setNotifyStep(0.01);
  const std::string reportMsg = "Computing corrections";

  bool enableParallelFor = true;
  for (auto &ws : simulationWSs) {
    enableParallelFor = enableParallelFor && Kernel::threadSafe(*ws);
  }
  enableParallelFor = enableParallelFor && Kernel::threadSafe(*noAbsOutputWS);

  const auto &spectrumInfo = instrumentWS.spectrumInfo();

  PARALLEL_FOR_IF(enableParallelFor)
  for (int64_t i = 0; i < nhists; ++i) {
    PARALLEL_START_INTERUPT_REGION
    auto &spectrum = instrumentWS.getSpectrum(i);
    Mantid::specnum_t specNo = spectrum.getSpectrumNo();
    MersenneTwister rng(seed + specNo);
    // no two theta for monitors

    if (spectrumInfo.hasDetectors(i) && !spectrumInfo.isMonitor(i) && !spectrumInfo.isMasked(i)) {

      const auto lambdas = instrumentWS.points(i).rawData();

      const auto nbins = lambdas.size();
      // step size = index range / number of steps requested
      const size_t nsteps = std::max(1, nlambda - 1);
      const size_t lambdaStepSize = nbins == 1 ? 1 : (nbins - 1) / nsteps;

      const auto detPos = spectrumInfo.position(i);

      for (size_t bin = 0; bin < nbins; bin += lambdaStepSize) {

        const double kinc = 2 * M_PI / lambdas[bin];

        double total =
            simulatePaths(nSingleScatterEvents, 1, sample, *instrument, rng, sigmaSSWS, SQHist, kinc, detPos, true);
        noAbsSimulationWS->getSpectrum(i).dataY()[bin] = total;

        for (int ne = 0; ne < nScatters; ne++) {
          int nEvents = ne == 0 ? nSingleScatterEvents : nMultiScatterEvents;

          total = simulatePaths(nEvents, ne + 1, sample, *instrument, rng, sigmaSSWS, SQHist, kinc, detPos, false);
          simulationWSs[ne]->getSpectrum(i).dataY()[bin] = total;
        }

        prog.report(reportMsg);

        // Ensure we have the last point for the interpolation
        if (lambdaStepSize > 1 && bin + lambdaStepSize >= nbins && bin + 1 != nbins) {
          bin = nbins - lambdaStepSize - 1;
        }
      } // bins

      // interpolate through points not simulated. Simulation WS only has
      // reduced X values if using sparse instrument so no interpolation
      // required
      if (!useSparseInstrument && lambdaStepSize > 1) {
        auto histnew = noAbsSimulationWS->histogram(i);
        if (lambdaStepSize < nbins) {
          interpolateOpt.applyInplace(histnew, lambdaStepSize);
        } else {
          std::fill(histnew.mutableY().begin() + 1, histnew.mutableY().end(), histnew.y()[0]);
        }
        noAbsOutputWS->setHistogram(i, histnew);

        for (size_t ne = 0; ne < static_cast<size_t>(nScatters); ne++) {
          auto histnew = simulationWSs[ne]->histogram(i);
          if (lambdaStepSize < nbins) {
            interpolateOpt.applyInplace(histnew, lambdaStepSize);
          } else {
            std::fill(histnew.mutableY().begin() + 1, histnew.mutableY().end(), histnew.y()[0]);
          }
          outputWSs[ne]->setHistogram(i, histnew);
        }
      }
    }

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  if (useSparseInstrument) {
    Poco::Thread::sleep(200); // to ensure prog message changes
    const std::string reportMsg = "Spatial Interpolation";
    prog.report(reportMsg);
    interpolateFromSparse(*noAbsOutputWS, *std::dynamic_pointer_cast<SparseWorkspace>(noAbsSimulationWS),
                          interpolateOpt);
    for (size_t ne = 0; ne < static_cast<size_t>(nScatters); ne++) {
      interpolateFromSparse(*outputWSs[ne], *std::dynamic_pointer_cast<SparseWorkspace>(simulationWSs[ne]),
                            interpolateOpt);
    }
  }

  // Create workspace group that holds output workspaces
  auto wsgroup = std::make_shared<WorkspaceGroup>();
  auto outputGroupWSName = getPropertyValue("OutputWorkspace");
  if (AnalysisDataService::Instance().doesExist(outputGroupWSName))
    API::AnalysisDataService::Instance().deepRemoveGroup(outputGroupWSName);

  const std::string wsNamePrefix = "Scatter_";
  std::string wsName = wsNamePrefix + "1_NoAbs";
  setWorkspaceName(noAbsOutputWS, wsName);
  wsgroup->addWorkspace(noAbsOutputWS);

  for (size_t i = 0; i < outputWSs.size(); i++) {
    wsName = wsNamePrefix + std::to_string(i + 1);
    setWorkspaceName(outputWSs[i], wsName);
    wsgroup->addWorkspace(outputWSs[i]);
  }

  if (outputWSs.size() > 1) {
    auto summedOutput = createOutputWorkspace(*inputWS);
    for (size_t i = 1; i < outputWSs.size(); i++) {
      summedOutput = summedOutput + outputWSs[i];
    }
    wsName = "Scatter_2_" + std::to_string(outputWSs.size()) + "_Summed";
    setWorkspaceName(summedOutput, wsName);
    wsgroup->addWorkspace(summedOutput);
  }

  // set the output property
  setProperty("OutputWorkspace", wsgroup);

  if (g_log.is(Kernel::Logger::Priority::PRIO_WARNING)) {
    for (auto &kv : m_attemptsToGenerateInitialTrack)
      g_log.warning() << "Generating initial track required " + std::to_string(kv.first) + " attempts on " +
                             std::to_string(kv.second) + " occasions.\n";
    g_log.warning() << "Calls to interceptSurface= " + std::to_string(m_callsToInterceptSurface) + "\n";
  }
}

/**
 * Calculate a total cross section using a k-specific scattering cross section
 * Note - a separate tabulated scattering cross section is used elsewhere in the
 * calculation
 * @param sigmaSSWS Workspace containing log of scattering cross section as a
 * function of k
 * @param material The sample material
 * @param kinc The incident wavenumber
 * @param specialSingleScatterCalc Boolean indicating whether special single
 * scatter calculation should be performed
 * @return The total cross section
 */
double CalculateMultipleScattering::new_vector(const MatrixWorkspace_sptr sigmaSSWS, const Material &material,
                                               double kinc, bool specialSingleScatterCalc) {
  double scatteringXSection, absorbXsection;
  if (specialSingleScatterCalc) {
    absorbXsection = 0;
  } else {
    const double wavelength = 2 * M_PI / kinc;
    absorbXsection = material.absorbXSection(wavelength);
  }
  if (sigmaSSWS) {
    scatteringXSection = interpolateGaussian(sigmaSSWS->histogram(0), kinc);
  } else {
    scatteringXSection = material.totalScatterXSection();
  }

  const auto sig_total = scatteringXSection + absorbXsection;
  return sig_total;
}

/**
 * Interpolate a value from a spectrum containing Gaussian peaks. The log of the spectrum has previously
  been taken so this method does a quadratic interpolation and returns e^y
 * @param histToInterpolate The histogram containing the data to interpolate
 * @param x The x value to interpolate at
 * @return The exponential of the interpolated value
 */
double CalculateMultipleScattering::interpolateGaussian(const HistogramData::Histogram &histToInterpolate, double x) {
  // could have written using points() method so it also worked on histogram data but found that the points
  // method was bottleneck on multithreaded code due to cow_ptr atomic_load
  assert(histToInterpolate.xMode() == HistogramData::Histogram::XMode::Points);
  if (x > histToInterpolate.x().back()) {
    return exp(histToInterpolate.y().back());
  }
  if (x < histToInterpolate.x().front()) {
    return exp(histToInterpolate.y().front());
  }
  // assume log(cross section) is quadratic in k
  auto deltax = histToInterpolate.x()[1] - histToInterpolate.x()[0];

  auto iter = std::upper_bound(histToInterpolate.x().cbegin(), histToInterpolate.x().cend(), x);
  auto idx = static_cast<size_t>(std::distance(histToInterpolate.x().cbegin(), iter) - 1);

  // need at least two points to the right of the x value for the quadratic
  // interpolation to work
  auto ny = histToInterpolate.y().size();
  if (ny < 3) {
    throw std::runtime_error("Need at least 3 y values to perform quadratic interpolation");
  }
  if (idx > ny - 3) {
    idx = ny - 3;
  }
  // this interpolation assumes the set of 3 bins\point have the same width
  // U=0 on point or bin edge to the left of where x lies
  const auto U = (x - histToInterpolate.x()[idx]) / deltax;
  const auto &y = histToInterpolate.y();
  const auto A = (y[idx] - 2 * y[idx + 1] + y[idx + 2]) / 2;
  const auto B = (-3 * y[idx] + 4 * y[idx + 1] - y[idx + 2]) / 2;
  const auto C = y[idx];
  return exp(A * U * U + B * U + C);
}

/**
 * Simulates a set of neutron paths through the sample to a specific detector
 * position with each path containing the specified number of scattering events.
 * Each path represents a group of neutrons and the proportion of neutrons
 * making it to the destination without being scattered or absorbed is
 * calculated as a weight using the cross section information from the sample
 * material. The average weight across all the simulated paths is returned
 * @param nPaths The number of paths to simulate
 * @param nScatters The number of scattering events to simulate along each path
 * @param sample A sample object
 * @param instrument An instrument object used to obtain the source position
 * @param rng Random number generator
 * @param sigmaSSWS
 * @param SOfQ
 * @param kinc
 * @param detPos
 * @param specialSingleScatterCalc Boolean indicating whether special single
 * scatter calculation should be performed
 * @return An average weight across all of the paths
 */
double CalculateMultipleScattering::simulatePaths(const int nPaths, const int nScatters, const Sample &sample,
                                                  const Geometry::Instrument &instrument,
                                                  Kernel::PseudoRandomNumberGenerator &rng,
                                                  const MatrixWorkspace_sptr sigmaSSWS,
                                                  const HistogramData::Histogram &SOfQ, const double kinc,
                                                  Kernel::V3D detPos, bool specialSingleScatterCalc) {
  double sumOfWeights = 0, sumOfQSS = 0.;
  auto sourcePos = instrument.getSource()->getPos();

  double sigma_total;
  double scatteringXSection = sample.getMaterial().totalScatterXSection();
  sigma_total = new_vector(sigmaSSWS, sample.getMaterial(), kinc, specialSingleScatterCalc);

  for (int ie = 0; ie < nPaths; ie++) {
    auto [success, weight, QSS] = scatter(nScatters, sample, instrument, sourcePos, rng, sigma_total,
                                          scatteringXSection, SOfQ, kinc, detPos, specialSingleScatterCalc);
    if (success) {
      sumOfWeights += weight;
      sumOfQSS += QSS;
    } else
      ie--;
  }

  // divide by the mean of Q*S(Q) for each of the n-1 terms representing a
  // multiple scatter
  sumOfWeights = sumOfWeights / pow(sumOfQSS / static_cast<double>(nPaths * (nScatters - 1)), nScatters - 1);

  return sumOfWeights / nPaths;
}

/**
 * Simulates a single neutron path through the sample to a specific detector
 * position containing the specified number of scattering events.
 * Each path represents a group of neutrons and the proportion of neutrons
 * making it to the destination without being scattered or absorbed is
 * calculated as a weight using the cross section information from the sample
 * material
 * @param nScatters The number of scattering events to simulate along each path
 * @param sample A sample object
 * @param instrument An instrument object used to obtain the reference frame
 * @param sourcePos The source xyz coordinates
 * @param rng Random number generator
 * @param sigma_total
 * @param scatteringXSection
 * @param SOfQ Pointer to workspace containing log(S(Q))
 * @param kinc The incident wavevector
 * @param detPos The detector position xyz coordinates
 * @param specialSingleScatterCalc Boolean indicating whether special single
 * scatter calculation should be performed
 * @return A tuple containing a success/fail boolean, the calculated weight and
 * a sum of the QSS values across the n-1 multiple scatters
 */
std::tuple<bool, double, double> CalculateMultipleScattering::scatter(
    const int nScatters, const Sample &sample, const Geometry::Instrument &instrument, const V3D sourcePos,
    Kernel::PseudoRandomNumberGenerator &rng, const double sigma_total, double scatteringXSection,
    const HistogramData::Histogram &SOfQ, const double kinc, Kernel::V3D detPos, bool specialSingleScatterCalc) {
  double weight = 1;
  double numberDensity = sample.getMaterial().numberDensityEffective();
  // if scale up scatteringXSection by 100*numberDensity then may not need
  // sigma_total any more but leave it alone now to align with original code
  double vmu = 100 * numberDensity * sigma_total;
  auto track = start_point(sample.getShape(), instrument.getReferenceFrame(), sourcePos, rng);
  updateWeightAndPosition(track, weight, vmu, sigma_total, rng);

  double QSS = 0;
  for (int iScat = 0; iScat < nScatters - 1; iScat++) {
    q_dir(track, SOfQ, kinc, scatteringXSection, rng, QSS, weight);
    const int nlinks = sample.getShape().interceptSurface(track);
    m_callsToInterceptSurface++;
    if (nlinks == 0) {
      return {false, 0, 0};
    }
    updateWeightAndPosition(track, weight, vmu, sigma_total, rng);
  }

  Kernel::V3D directionToDetector = detPos - track.startPoint();
  Kernel::V3D prevDirection = track.direction();
  directionToDetector.normalize();
  track.reset(track.startPoint(), directionToDetector);
  const int nlinks = sample.getShape().interceptSurface(track);
  m_callsToInterceptSurface++;
  // due to VALID_INTERCEPT_POINT_SHIFT some tracks that skim the surface
  // of a CSGObject sample may not generate valid tracks. Start over again
  // for this event
  if (nlinks == 0) {
    return {false, 0, 0};
  }
  const double dl = track.front().distInsideObject;
  const auto q = (directionToDetector - prevDirection) * kinc;
  const auto SQ = interpolateGaussian(SOfQ, q.norm());
  if (specialSingleScatterCalc)
    vmu = 0;
  const auto AT2 = exp(-dl * vmu);
  weight = weight * AT2 * SQ * scatteringXSection / (4 * M_PI);
  return {true, weight, QSS};
}

// update track direction, QSS and weight
void CalculateMultipleScattering::q_dir(Geometry::Track &track, const HistogramData::Histogram &SOfQ, const double kinc,
                                        const double scatteringXSection, Kernel::PseudoRandomNumberGenerator &rng,
                                        double &QSS, double &weight) {
  const double kf = kinc; // elastic only so far
  const double qmin = abs(kf - kinc);
  const double qrange = 2 * kinc;

  const double QQ = qmin + qrange * rng.nextValue();
  // T = 2theta
  const double cosT = (kinc * kinc + kf * kf - QQ * QQ) / (2 * kinc * kf);

  const double SQ = interpolateGaussian(SOfQ, QQ);
  QSS += QQ * SQ;
  weight = weight * scatteringXSection * SQ * QQ;
  updateTrackDirection(track, cosT, rng.nextValue() * 2 * M_PI);
}

/**
 * Update the track's direction following a scatter event given theta and phi angles
 * @param track The track whose direction will be updated
 * @param cosT Cos two theta. two theta is scattering angle
 * @param phi Phi (radians) of after track. Measured in plane perpendicular to initial trajectory
 */
void CalculateMultipleScattering::updateTrackDirection(Geometry::Track &track, const double cosT, const double phi) {
  const auto B3 = sqrt(1 - cosT * cosT);
  const auto B2 = cosT;
  // possible to do this using the Quat class instead??
  // Quat(const double _deg, const V3D &_axis);
  // Quat(acos(cosT)*180/M_PI,
  // Kernel::V3D(track.direction()[],track.direction()[],0))

  // Rodrigues formula with final term equal to zero
  // v_rot = cosT * v + sinT(k x v)
  // with rotation axis k orthogonal to v
  // Define k by first creating two vectors orthogonal to v:
  // (-vy, vx, 0) by inspection
  // and then (-vz * vx, -vy * vz, vx * vx + vy * vy) as cross product
  // Then define k as combination of these:
  // sin(phi) * (vy, -vx, 0) + cos(phi) * (-vx * vz, -vy * vz, 1 - vz * vz)
  // Note: xyz convention here isn't the standard Mantid one. x=beam, z=up
  const auto vy = track.direction()[0];
  const auto vz = track.direction()[1];
  const auto vx = track.direction()[2];
  double UKX, UKY, UKZ;
  if (vz < 1) {
    auto A2 = sqrt(1 - vz * vz);
    auto UQTZ = cos(phi) * A2;
    auto UQTX = -cos(phi) * vz * vx / A2 + sin(phi) * vy / A2;
    auto UQTY = -cos(phi) * vz * vy / A2 - sin(phi) * vx / A2;
    UKX = B2 * vx + B3 * UQTX;
    UKY = B2 * vy + B3 * UQTY;
    UKZ = B2 * vz + B3 * UQTZ;
  } else {
    UKX = B3 * cos(phi);
    UKY = B3 * sin(phi);
    UKZ = B2;
  }
  track.reset(track.startPoint(), Kernel::V3D(UKY, UKZ, UKX));
}

/**
 * Repeatedly attempt to generate an initial track starting at the source and entering the sample at a random point on
 * its front surface. After each attempt check the track has at least one intercept with sample shape (sometimes for
 * tracks very close to the surface this can sometimes be zero due to numerical precision)
 * @param shape The sample shape
 * @param frame The instrument's reference frame
 * @param sourcePos The source position
 * @param rng Random number generator
 * @return a track intercepting the sample
 */
Geometry::Track CalculateMultipleScattering::start_point(const Geometry::IObject &shape,
                                                         std::shared_ptr<const Geometry::ReferenceFrame> frame,
                                                         const V3D sourcePos,
                                                         Kernel::PseudoRandomNumberGenerator &rng) {
  const int MAX_ATTEMPTS = 100;
  for (int i = 0; i < MAX_ATTEMPTS; i++) {
    auto t = generateInitialTrack(shape, frame, sourcePos, rng);
    const int nlinks = shape.interceptSurface(t);
    m_callsToInterceptSurface++;
    if (nlinks > 0) {
      if (i > 0) {
        if (g_log.is(Kernel::Logger::Priority::PRIO_WARNING)) {
          m_attemptsToGenerateInitialTrack[i + 1]++;
        }
      }
      return t;
    }
  }
  throw std::runtime_error("CalculateMultipleScattering::start_point() - Unable to "
                           "generate entry point into sample");
}

// update track start point and weight
void CalculateMultipleScattering::updateWeightAndPosition(Geometry::Track &track, double &weight, const double vmu,
                                                          const double sigma_total,
                                                          Kernel::PseudoRandomNumberGenerator &rng) {
  const double dl = track.front().distInsideObject;
  const double b4 = (1.0 - exp(-dl * vmu));
  const double vmfp = 1.0 / vmu;
  const double vl = -(vmfp * log(1 - rng.nextValue() * b4));
  weight = weight * b4 / sigma_total;
  inc_xyz(track, vl);
}

/**
 * Generate an initial track starting at the source and entering
 * the sample at a random point on its front surface
 * @param shape The sample shape
 * @param frame The instrument's reference frame
 * @param sourcePos The source position
 * @param rng Random number generator
 * @return a track
 */
Geometry::Track CalculateMultipleScattering::generateInitialTrack(const Geometry::IObject &shape,
                                                                  std::shared_ptr<const Geometry::ReferenceFrame> frame,
                                                                  const V3D &sourcePos,
                                                                  Kernel::PseudoRandomNumberGenerator &rng) {
  auto sampleBox = shape.getBoundingBox();
  // generate random point on front surface of sample bounding box
  // I'm not 100% sure this sampling is correct because for a sample with
  // varying thickness (eg cylinder) you end up with more entry points in the
  // thin part of the cylinder
  auto ptx = sampleBox.minPoint()[frame->pointingHorizontal()] +
             rng.nextValue() * sampleBox.width()[frame->pointingHorizontal()];
  auto pty = sampleBox.minPoint()[frame->pointingUp()] + rng.nextValue() * sampleBox.width()[frame->pointingUp()];
  // perhaps eventually also generate random point on the beam profile?
  auto ptOnBeamProfile = Kernel::V3D();
  ptOnBeamProfile[frame->pointingHorizontal()] = ptx;
  ptOnBeamProfile[frame->pointingUp()] = pty;
  ptOnBeamProfile[frame->pointingAlongBeam()] = sourcePos[frame->pointingAlongBeam()];
  auto toSample = Kernel::V3D();
  toSample[frame->pointingAlongBeam()] = 1.;
  return Geometry::Track(ptOnBeamProfile, toSample);
}

/**
 * Update the x, y, z position of the neutron (or dV volume element
 * to integrate over). Save new start point in to the track object
 * supplied along
 * @param track A track defining the current trajectory
 * @param vl A distance to move along the current trajectory
 */
void CalculateMultipleScattering::inc_xyz(Geometry::Track &track, double vl) {
  Kernel::V3D position = track.front().entryPoint;
  Kernel::V3D direction = track.direction();
  auto x = position[0] + vl * direction[0];
  auto y = position[1] + vl * direction[1];
  auto z = position[2] + vl * direction[2];
  auto startPoint = track.startPoint();
  startPoint = V3D(x, y, z);
  track.clearIntersectionResults();
  track.reset(startPoint, track.direction());
}

/**
 * Factory method to return an instance of the required SparseInstrument class
 * @param modelWS The full workspace that the sparse one will be based on
 * @param wavelengthPoints The number of wavelength points to include in the
 * histograms in the sparse workspace
 * @param rows The number of rows of detectors to create
 * @param columns The number of columns of detectors to create
 * @return a pointer to an SparseInstrument object
 */
std::shared_ptr<SparseWorkspace> CalculateMultipleScattering::createSparseWorkspace(const API::MatrixWorkspace &modelWS,
                                                                                    const size_t wavelengthPoints,
                                                                                    const size_t rows,
                                                                                    const size_t columns) {
  auto sparseWS = std::make_shared<SparseWorkspace>(modelWS, wavelengthPoints, rows, columns);
  return sparseWS;
}

MatrixWorkspace_sptr CalculateMultipleScattering::createOutputWorkspace(const MatrixWorkspace &inputWS) const {
  MatrixWorkspace_uptr outputWS = DataObjects::create<Workspace2D>(inputWS);
  // The algorithm computes the signal values at bin centres so they should
  // be treated as a distribution
  outputWS->setDistribution(true);
  outputWS->setYUnit("");
  outputWS->setYUnitLabel("Scattered Weight");
  return outputWS;
}

/**
 * Factory method to return an instance of the required InterpolationOption
 * class
 * @return a pointer to an InterpolationOption object
 */
std::unique_ptr<InterpolationOption> CalculateMultipleScattering::createInterpolateOption() {
  auto interpolationOpt = std::make_unique<InterpolationOption>();
  return interpolationOpt;
}

void CalculateMultipleScattering::interpolateFromSparse(MatrixWorkspace &targetWS, const SparseWorkspace &sparseWS,
                                                        const Mantid::Algorithms::InterpolationOption &interpOpt) {
  const auto &spectrumInfo = targetWS.spectrumInfo();
  const auto refFrame = targetWS.getInstrument()->getReferenceFrame();
  PARALLEL_FOR_IF(Kernel::threadSafe(targetWS, sparseWS))
  for (int64_t i = 0; i < static_cast<decltype(i)>(spectrumInfo.size()); ++i) {
    PARALLEL_START_INTERUPT_REGION
    if (!spectrumInfo.isMonitor(i)) {
      double lat, lon;
      std::tie(lat, lon) = spectrumInfo.geographicalAngles(i);
      const auto spatiallyInterpHisto = sparseWS.bilinearInterpolateFromDetectorGrid(lat, lon);
      if (spatiallyInterpHisto.size() > 1) {
        auto targetHisto = targetWS.histogram(i);
        interpOpt.applyInPlace(spatiallyInterpHisto, targetHisto);
        targetWS.setHistogram(i, targetHisto);
      } else {
        targetWS.mutableY(i) = spatiallyInterpHisto.y().front();
      }
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

void CalculateMultipleScattering::correctForWorkspaceNameClash(std::string &wsName) {
  bool noClash(false);

  for (int i = 0; !noClash; ++i) {
    std::string wsIndex; // dont use an index if there is no other
                         // workspace
    if (i > 0) {
      wsIndex = "_" + std::to_string(i);
    }

    bool wsExists = AnalysisDataService::Instance().doesExist(wsName + wsIndex);
    if (!wsExists) {
      wsName += wsIndex;
      noClash = true;
    }
  }
}

/**
 * Set the name on a workspace, adjusting for potential clashes in the ADS.
 * Used to set the names on the output workspace group members. N
 * @param ws The ws to set the name on
 * @param wsName The name to set on the workspace
 */
void CalculateMultipleScattering::setWorkspaceName(const API::MatrixWorkspace_sptr &ws, std::string wsName) {
  correctForWorkspaceNameClash(wsName);
  API::AnalysisDataService::Instance().addOrReplace(wsName, ws);
}

} // namespace Algorithms
} // namespace Mantid