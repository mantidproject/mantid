// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/MonteCarloAbsorption.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAlgorithms/BeamProfileFactory.h"
#include "MantidAlgorithms/InterpolationOption.h"
#include "MantidAlgorithms/SampleCorrections/DetectorGridDefinition.h"
#include "MantidAlgorithms/SampleCorrections/MCInteractionStatistics.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/DeltaEMode.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MersenneTwister.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/VectorHelper.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using Mantid::DataObjects::Workspace2D;
namespace PhysicalConstants = Mantid::PhysicalConstants;

/// @cond
namespace {

constexpr int DEFAULT_NEVENTS = 1000;
constexpr int DEFAULT_SEED = 123456789;
constexpr int DEFAULT_LATITUDINAL_DETS = 5;
constexpr int DEFAULT_LONGITUDINAL_DETS = 10;

/// Energy (meV) to wavelength (angstroms)
inline double toWavelength(double energy) {
  static const double factor =
      1e10 * PhysicalConstants::h / sqrt(2.0 * PhysicalConstants::NeutronMass * PhysicalConstants::meV);
  return factor / sqrt(energy);
}

struct EFixedProvider {
  explicit EFixedProvider(const ExperimentInfo &expt) : m_expt(expt), m_emode(expt.getEMode()), m_value(0.0) {
    if (m_emode == DeltaEMode::Direct) {
      m_value = m_expt.getEFixed();
    }
  }
  inline DeltaEMode::Type emode() const { return m_emode; }
  inline double value(const Mantid::detid_t detID) const {
    if (m_emode != DeltaEMode::Indirect)
      return m_value;
    else
      return m_expt.getEFixed(detID);
  }

private:
  const ExperimentInfo &m_expt;
  const DeltaEMode::Type m_emode;
  double m_value;
};
} // namespace
/// @endcond

namespace Mantid::Algorithms {

DECLARE_ALGORITHM(MonteCarloAbsorption)

//------------------------------------------------------------------------------
// Private methods
//------------------------------------------------------------------------------

/**
 * Initialize the algorithm
 */
void MonteCarloAbsorption::init() {
  // The input workspace must have an instrument and units of wavelength
  auto wsValidator = std::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<InstrumentValidator>();

  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, wsValidator),
                  "The name of the input workspace.  The input workspace must "
                  "have X units of wavelength.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The name to use for the output workspace.");

  auto positiveInt = std::make_shared<Kernel::BoundedValidator<int>>();
  positiveInt->setLower(1);
  declareProperty("NumberOfWavelengthPoints", EMPTY_INT(), positiveInt,
                  "The number of wavelength points for which a simulation is "
                  "attempted if ResimulateTracksForDifferentWavelengths=true");
  declareProperty("EventsPerPoint", DEFAULT_NEVENTS, positiveInt,
                  "The number of \"neutron\" events to generate per simulated point");
  declareProperty("SeedValue", DEFAULT_SEED, positiveInt, "Seed the random number generator with this value");

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

  // Control the number of attempts made to generate a random point in the
  // object
  declareProperty("MaxScatterPtAttempts", 5000, positiveInt,
                  "Maximum number of tries made to generate a scattering point "
                  "within the sample (+ optional container etc). Objects with "
                  "holes in them, e.g. a thin annulus can cause problems "
                  "if this number is too low.\n"
                  "If a scattering point cannot be generated by increasing "
                  "this value then there is most likely a problem with "
                  "the sample geometry.");
  declareProperty("ResimulateTracksForDifferentWavelengths", false, "Resimulate tracks for each wavelength point.");
  setPropertySettings("NumberOfWavelengthPoints",
                      std::make_unique<EnabledWhenProperty>("ResimulateTracksForDifferentWavelengths",
                                                            ePropertyCriterion::IS_NOT_DEFAULT));
  auto scatteringOptionValidator = std::make_shared<StringListValidator>();
  scatteringOptionValidator->addAllowedValue("SampleAndEnvironment");
  scatteringOptionValidator->addAllowedValue("SampleOnly");
  scatteringOptionValidator->addAllowedValue("EnvironmentOnly");
  declareProperty("SimulateScatteringPointIn", "SampleAndEnvironment",
                  "Simulate the scattering point in the vicinity of the sample or its "
                  "environment or both (default).",
                  scatteringOptionValidator);
}

/**
 * Execution code
 */
void MonteCarloAbsorption::exec() {
  const MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  const int nevents = getProperty("EventsPerPoint");
  const bool resimulateTracks = getProperty("ResimulateTracksForDifferentWavelengths");
  const int seed = getProperty("SeedValue");
  InterpolationOption interpolateOpt;
  interpolateOpt.set(getPropertyValue("Interpolation"), true, resimulateTracks);
  const bool useSparseInstrument = getProperty("SparseInstrument");
  const int maxScatterPtAttempts = getProperty("MaxScatterPtAttempts");
  auto simulatePointsIn = MCInteractionVolume::ScatteringPointVicinity::SAMPLEANDENVIRONMENT;
  const auto pointsInProperty = getPropertyValue("SimulateScatteringPointIn");
  if (pointsInProperty == "SampleOnly") {
    simulatePointsIn = MCInteractionVolume::ScatteringPointVicinity::SAMPLEONLY;
  } else if (pointsInProperty == "EnvironmentOnly") {
    simulatePointsIn = MCInteractionVolume::ScatteringPointVicinity::ENVIRONMENTONLY;
  }
  auto outputWS = doSimulation(*inputWS, static_cast<size_t>(nevents), resimulateTracks, seed, interpolateOpt,
                               useSparseInstrument, static_cast<size_t>(maxScatterPtAttempts), simulatePointsIn);
  setProperty("OutputWorkspace", std::move(outputWS));
}

/**
 * Validate the input properties.
 * @return a map where keys are property names and values the found issues
 */
std::map<std::string, std::string> MonteCarloAbsorption::validateInputs() {
  std::map<std::string, std::string> issues;
  const bool resimulateTracksForDiffWavelengths = getProperty("ResimulateTracksForDifferentWavelengths");
  // Only interpolate between wavelength points if resimulating tracks
  if (resimulateTracksForDiffWavelengths) {
    const int nlambda = getProperty("NumberOfWavelengthPoints");
    InterpolationOption interpOpt;
    const std::string interpValue = getPropertyValue("Interpolation");
    interpOpt.set(interpValue, true, resimulateTracksForDiffWavelengths);
    const auto nlambdaIssue = interpOpt.validateInputSize(nlambda);
    if (!nlambdaIssue.empty()) {
      issues["NumberOfWavelengthPoints"] = nlambdaIssue;
    }
  }
  return issues;
}

/**
 * Factory method to return an instance of the required interaction volume
 * class
 * @param sample A reference to the object defining details of the sample
 * @param maxScatterPtAttempts The maximum number of tries to generate a random
 * point within the object
 * @param pointsIn Where to generate the scattering point in
 * @return a pointer to an MCAbsorptionStrategy object
 */
std::shared_ptr<IMCInteractionVolume>
MonteCarloAbsorption::createInteractionVolume(const API::Sample &sample, const size_t maxScatterPtAttempts,
                                              const MCInteractionVolume::ScatteringPointVicinity pointsIn,
                                              const Geometry::IObject_sptr gaugeVolume) {
  auto interactionVol = std::make_shared<MCInteractionVolume>(sample, maxScatterPtAttempts, pointsIn);
  return interactionVol;
}

/**
 * Factory method to return an instance of the required absorption strategy
 * class
 * @param interactionVol The interaction volume object to inject into the
 * strategy
 * @param beamProfile A reference to the object the beam profile
 * @param EMode The energy mode of the instrument
 * @param nevents The number of Monte Carlo events used in the simulation
 * @param maxScatterPtAttempts The maximum number of tries to generate a random
 * point within the object
 * @param regenerateTracksForEachLambda Whether to resimulate tracks for each
 * wavelength point or not
 * @return a pointer to an MCAbsorptionStrategy object
 */
std::shared_ptr<IMCAbsorptionStrategy>
MonteCarloAbsorption::createStrategy(IMCInteractionVolume &interactionVol, const IBeamProfile &beamProfile,
                                     Kernel::DeltaEMode::Type EMode, const size_t nevents,
                                     const size_t maxScatterPtAttempts, const bool regenerateTracksForEachLambda) {
  auto MCAbs = std::make_shared<MCAbsorptionStrategy>(interactionVol, beamProfile, EMode, nevents, maxScatterPtAttempts,
                                                      regenerateTracksForEachLambda);
  return MCAbs;
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
std::shared_ptr<SparseWorkspace> MonteCarloAbsorption::createSparseWorkspace(const API::MatrixWorkspace &modelWS,
                                                                             const size_t wavelengthPoints,
                                                                             const size_t rows, const size_t columns) {
  auto sparseWS = std::make_shared<SparseWorkspace>(modelWS, wavelengthPoints, rows, columns);
  return sparseWS;
}

/**
 * Factory method to return an instance of the required InterpolationOption
 * class
 * @return a pointer to an InterpolationOption object
 */
std::unique_ptr<InterpolationOption> MonteCarloAbsorption::createInterpolateOption() {
  auto interpolationOpt = std::make_unique<InterpolationOption>();
  return interpolationOpt;
}

/**
 * Run the simulation over the whole input workspace
 * @param inputWS A reference to the input workspace
 * @param nevents Number of MC events per wavelength point to simulate
 * @param resimulateTracksForDiffWavelengths Whether to resimulate the tracks
 * for each wavelength point
 * @param seed Seed value for the random number generator
 * @param interpolateOpt Method of interpolation to compute unsimulated points
 * @param useSparseInstrument If true, use sparse instrument in simulation
 * @param maxScatterPtAttempts The maximum number of tries to generate a
 * scatter point within the object
 * @param pointsIn Where to simulate the scattering point in
 * @return A new workspace containing the correction factors & errors
 */
MatrixWorkspace_uptr MonteCarloAbsorption::doSimulation(const MatrixWorkspace &inputWS, const size_t nevents,
                                                        const bool resimulateTracksForDiffWavelengths, const int seed,
                                                        const InterpolationOption &interpolateOpt,
                                                        const bool useSparseInstrument,
                                                        const size_t maxScatterPtAttempts,
                                                        const MCInteractionVolume::ScatteringPointVicinity pointsIn) {
  auto outputWS = createOutputWorkspace(inputWS);
  const auto inputNbins = static_cast<int>(inputWS.blocksize());

  int nlambda;
  if (resimulateTracksForDiffWavelengths) {
    nlambda = getProperty("NumberOfWavelengthPoints");
    if (isEmpty(nlambda) || nlambda > inputNbins) {
      if (!isEmpty(nlambda)) {
        g_log.warning() << "The requested number of wavelength points is larger "
                           "than the spectra size. "
                           "Defaulting to spectra size.\n";
      }
      nlambda = inputNbins;
    }
  } else {
    nlambda = inputNbins;
  }
  SparseWorkspace_sptr sparseWS;
  if (useSparseInstrument) {
    const int latitudinalDets = getProperty("NumberOfDetectorRows");
    const int longitudinalDets = getProperty("NumberOfDetectorColumns");
    sparseWS = createSparseWorkspace(inputWS, nlambda, latitudinalDets, longitudinalDets);
  }
  MatrixWorkspace &simulationWS = useSparseInstrument ? *sparseWS : *outputWS;
  const MatrixWorkspace &instrumentWS = useSparseInstrument ? simulationWS : inputWS;
  // Cache information about the workspace that will be used repeatedly
  auto instrument = instrumentWS.getInstrument();
  const auto nhists = static_cast<int64_t>(instrumentWS.getNumberHistograms());

  EFixedProvider efixed(instrumentWS);
  auto beamProfile = BeamProfileFactory::createBeamProfile(*instrument, inputWS.sample());

  // Configure progress
  Progress prog(this, 0.0, 1.0, nhists);
  prog.setNotifyStep(0.01);
  const std::string reportMsg = "Computing corrections";

  // Configure strategy
  Geometry::IObject_sptr gaugeVolume = nullptr;
  if (inputWS.run().hasProperty("GaugeVolume")) {
    Geometry::IObject_sptr gaugeVolume = ShapeFactory().createShape(inputWS.run().getProperty("GaugeVolume")->value());
  }
  auto interactionVolume = createInteractionVolume(inputWS.sample(), maxScatterPtAttempts, pointsIn, gaugeVolume);
  auto strategy = createStrategy(*interactionVolume, *beamProfile, efixed.emode(), nevents, maxScatterPtAttempts,
                                 resimulateTracksForDiffWavelengths);

  const auto &spectrumInfo = simulationWS.spectrumInfo();

  PARALLEL_FOR_IF(Kernel::threadSafe(simulationWS))
  for (int64_t i = 0; i < nhists; ++i) {
    PARALLEL_START_INTERRUPT_REGION

    auto &outE = simulationWS.mutableE(i);
    // The input was cloned so clear the errors out
    outE = 0.0;

    if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMasked(i)) {
      continue;
    }
    // Per spectrum values
    const auto &detPos = spectrumInfo.position(i);
    const double lambdaFixed = toWavelength(efixed.value(spectrumInfo.detector(i).getID()));
    MersenneTwister rng(seed + int(i));

    const auto lambdas = simulationWS.points(i).rawData();

    const auto nbins = lambdas.size();
    const size_t lambdaStepSize = nbins / nlambda;

    std::vector<double> packedLambdas;
    std::vector<double> packedAttFactors;
    std::vector<double> packedAttFactorErrors;

    for (size_t j = 0; j < nbins; j += lambdaStepSize) {
      packedLambdas.push_back(lambdas[j]);
      packedAttFactors.push_back(0);
      packedAttFactorErrors.push_back(0);
      // Ensure we have the last point for the interpolation
      if (lambdaStepSize > 1 && j + lambdaStepSize >= nbins && j + 1 != nbins) {
        j = nbins - lambdaStepSize - 1;
      }
    }
    MCInteractionStatistics detStatistics(spectrumInfo.detector(i).getID(), inputWS.sample());

    strategy->calculate(rng, detPos, packedLambdas, lambdaFixed, packedAttFactors, packedAttFactorErrors,
                        detStatistics);

    if (g_log.is(Kernel::Logger::Priority::PRIO_DEBUG)) {
      g_log.debug(detStatistics.generateScatterPointStats());
    }

    for (size_t j = 0; j < packedLambdas.size(); j++) {
      auto idx = simulationWS.yIndexOfX(packedLambdas[j], i);
      simulationWS.getSpectrum(i).dataY()[idx] = packedAttFactors[j];
      simulationWS.getSpectrum(i).dataE()[idx] = packedAttFactorErrors[j];
    }

    // Interpolate through points not simulated. Simulation WS only has
    // reduced X values if using sparse instrument so no interpolation required

    if (!useSparseInstrument && lambdaStepSize > 1) {
      auto histnew = simulationWS.histogram(i);

      if (lambdaStepSize < nbins) {
        interpolateOpt.applyInplace(histnew, lambdaStepSize);
      } else {
        std::fill(histnew.mutableY().begin() + 1, histnew.mutableY().end(), histnew.y()[0]);
      }
      outputWS->setHistogram(i, histnew);
    }

    prog.report(reportMsg);

    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  if (useSparseInstrument) {
    interpolateFromSparse(*outputWS, *sparseWS, interpolateOpt);
  }

  return outputWS;
}

MatrixWorkspace_uptr MonteCarloAbsorption::createOutputWorkspace(const MatrixWorkspace &inputWS) const {
  MatrixWorkspace_uptr outputWS = DataObjects::create<Workspace2D>(inputWS);
  // The algorithm computes the signal values at bin centres so they should
  // be treated as a distribution
  outputWS->setDistribution(true);
  outputWS->setYUnit("");
  outputWS->setYUnitLabel("Attenuation factor");
  return outputWS;
}

void MonteCarloAbsorption::interpolateFromSparse(MatrixWorkspace &targetWS, const SparseWorkspace &sparseWS,
                                                 const Mantid::Algorithms::InterpolationOption &interpOpt) {
  const auto &spectrumInfo = targetWS.spectrumInfo();
  const auto refFrame = targetWS.getInstrument()->getReferenceFrame();
  PARALLEL_FOR_IF(Kernel::threadSafe(targetWS, sparseWS))
  for (int64_t i = 0; i < static_cast<decltype(i)>(spectrumInfo.size()); ++i) {
    PARALLEL_START_INTERRUPT_REGION
    if (spectrumInfo.hasDetectors(i)) {
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
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
}
} // namespace Mantid::Algorithms
