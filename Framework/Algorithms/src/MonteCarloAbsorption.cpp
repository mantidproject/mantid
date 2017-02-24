#include "MantidAlgorithms/MonteCarloAbsorption.h"
#include "MantidAlgorithms/InterpolationOption.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAlgorithms/SampleCorrections/MCAbsorptionStrategy.h"
#include "MantidAlgorithms/SampleCorrections/RectangularBeamProfile.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"

#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/DeltaEMode.h"
#include "MantidKernel/MersenneTwister.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/VectorHelper.h"

#include "MantidHistogramData/HistogramX.h"
#include "MantidHistogramData/Interpolate.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using Mantid::HistogramData::HistogramX;
using Mantid::HistogramData::interpolateLinearInplace;
using Mantid::DataObjects::Workspace2D;
namespace PhysicalConstants = Mantid::PhysicalConstants;

/// @cond
namespace {

constexpr int DEFAULT_NEVENTS = 300;
constexpr int DEFAULT_SEED = 123456789;

/// Energy (meV) to wavelength (angstroms)
inline double toWavelength(double energy) {
  static const double factor =
      1e10 * PhysicalConstants::h /
      sqrt(2.0 * PhysicalConstants::NeutronMass * PhysicalConstants::meV);
  return factor / sqrt(energy);
}

struct EFixedProvider {
  explicit EFixedProvider(const ExperimentInfo &expt)
      : m_expt(expt), m_emode(expt.getEMode()), m_value(0.0) {
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
}
/// @endcond

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(MonteCarloAbsorption)

//------------------------------------------------------------------------------
// Private methods
//------------------------------------------------------------------------------

/**
 * Initialize the algorithm
 */
void MonteCarloAbsorption::init() {
  // The input workspace must have an instrument and units of wavelength
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<InstrumentValidator>();

  declareProperty(make_unique<WorkspaceProperty<>>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "The name of the input workspace.  The input workspace must "
                  "have X units of wavelength.");
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The name to use for the output workspace.");
  auto positiveInt = boost::make_shared<Kernel::BoundedValidator<int>>();
  positiveInt->setLower(1);
  declareProperty("NumberOfWavelengthPoints", EMPTY_INT(), positiveInt,
                  "The number of wavelength points for which a simulation is "
                  "atttempted (default: all points)");
  declareProperty(
      "EventsPerPoint", DEFAULT_NEVENTS, positiveInt,
      "The number of \"neutron\" events to generate per simulated point");
  declareProperty("SeedValue", DEFAULT_SEED, positiveInt,
                  "Seed the random number generator with this value");

  InterpolationOption interpolateOpt;
  declareProperty(interpolateOpt.property(), interpolateOpt.propertyDoc());
}

/**
 * Execution code
 */
void MonteCarloAbsorption::exec() {
  const MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  const int nevents = getProperty("EventsPerPoint");
  const int nlambda = getProperty("NumberOfWavelengthPoints");
  const int seed = getProperty("SeedValue");
  InterpolationOption interpolateOpt;
  interpolateOpt.set(getPropertyValue("Interpolation"));

  auto outputWS = doSimulation(*inputWS, static_cast<size_t>(nevents), nlambda,
                               seed, interpolateOpt);

  setProperty("OutputWorkspace", std::move(outputWS));
}

/**
 * Run the simulation over the whole input workspace
 * @param inputWS A reference to the input workspace
 * @param nevents Number of MC events per wavelength point to simulate
 * @param nlambda Number of wavelength points to simulate. The remainder
 * are computed using interpolation
 * @param seed Seed value for the random number generator
 * @param interpolateOpt Method of interpolation to compute unsimulated points
 * @return A new workspace containing the correction factors & errors
 */
MatrixWorkspace_uptr
MonteCarloAbsorption::doSimulation(const MatrixWorkspace &inputWS,
                                   size_t nevents, int nlambda, int seed,
                                   const InterpolationOption &interpolateOpt) {
  auto outputWS = createOutputWorkspace(inputWS);
  // Cache information about the workspace that will be used repeatedly
  auto instrument = inputWS.getInstrument();
  const int64_t nhists = static_cast<int64_t>(inputWS.getNumberHistograms());
  const int nbins = static_cast<int>(inputWS.blocksize());
  if (isEmpty(nlambda) || nlambda > nbins) {
    if (!isEmpty(nlambda)) {
      g_log.warning() << "The requested number of wavelength points is larger "
                         "than the spectra size. "
                         "Defaulting to spectra size.\n";
    }
    nlambda = nbins;
  }

  EFixedProvider efixed(inputWS);
  auto beamProfile = createBeamProfile(*instrument, inputWS.sample());

  // Configure progress
  const int lambdaStepSize = nbins / nlambda;
  Progress prog(this, 0.0, 1.0, nhists * nbins / lambdaStepSize);
  prog.setNotifyStep(0.01);
  const std::string reportMsg = "Computing corrections";

  // Configure strategy
  MCAbsorptionStrategy strategy(*beamProfile, inputWS.sample(), nevents);

  const auto &spectrumInfo = outputWS->spectrumInfo();

  PARALLEL_FOR_IF(Kernel::threadSafe(*outputWS))
  for (int64_t i = 0; i < nhists; ++i) {
    PARALLEL_START_INTERUPT_REGION

    auto &outE = outputWS->mutableE(i);
    // The input was cloned so clear the errors out
    outE = 0.0;
    // Final detector position
    if (!spectrumInfo.hasDetectors(i)) {
      continue;
    }
    // Per spectrum values
    const auto &detPos = spectrumInfo.position(i);
    const double lambdaFixed =
        toWavelength(efixed.value(spectrumInfo.detector(i).getID()));
    MersenneTwister rng(seed);

    auto &outY = outputWS->mutableY(i);
    const auto lambdas = outputWS->points(i);
    // Simulation for each requested wavelength point
    for (int j = 0; j < nbins; j += lambdaStepSize) {
      prog.report(reportMsg);
      const double lambdaStep = lambdas[j];
      double lambdaIn(lambdaStep), lambdaOut(lambdaStep);
      if (efixed.emode() == DeltaEMode::Direct) {
        lambdaIn = lambdaFixed;
      } else if (efixed.emode() == DeltaEMode::Indirect) {
        lambdaOut = lambdaFixed;
      } else {
        // elastic case already initialized
      }
      std::tie(outY[j], std::ignore) =
          strategy.calculate(rng, detPos, lambdaIn, lambdaOut);

      // Ensure we have the last point for the interpolation
      if (lambdaStepSize > 1 && j + lambdaStepSize >= nbins && j + 1 != nbins) {
        j = nbins - lambdaStepSize - 1;
      }
    }

    // Interpolate through points not simulated
    if (lambdaStepSize > 1) {
      auto histnew = outputWS->histogram(i);
      interpolateOpt.applyInplace(histnew, lambdaStepSize);
      outputWS->setHistogram(i, histnew);
    }

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  return outputWS;
}

MatrixWorkspace_uptr MonteCarloAbsorption::createOutputWorkspace(
    const MatrixWorkspace &inputWS) const {
  MatrixWorkspace_uptr outputWS = DataObjects::create<Workspace2D>(inputWS);
  // The algorithm computes the signal values at bin centres so they should
  // be treated as a distribution
  outputWS->setDistribution(true);
  outputWS->setYUnit("");
  outputWS->setYUnitLabel("Attenuation factor");
  return outputWS;
}

/**
 * Create the beam profile. Currently only supports Rectangular. The dimensions
 * are either specified by those provided by `SetBeam` algorithm or default
 * to the width and height of the samples bounding box
 * @param instrument A reference to the instrument object
 * @param sample A reference to the sample object
 * @return A new IBeamProfile object
 */
std::unique_ptr<IBeamProfile>
MonteCarloAbsorption::createBeamProfile(const Instrument &instrument,
                                        const Sample &sample) const {
  const auto frame = instrument.getReferenceFrame();
  const auto source = instrument.getSource();

  auto beamWidthParam = source->getNumberParameter("beam-width");
  auto beamHeightParam = source->getNumberParameter("beam-height");
  double beamWidth(-1.0), beamHeight(-1.0);
  if (beamWidthParam.size() == 1 && beamHeightParam.size() == 1) {
    beamWidth = beamWidthParam[0];
    beamHeight = beamHeightParam[0];
  } else {
    const auto bbox = sample.getShape().getBoundingBox().width();
    beamWidth = bbox[frame->pointingHorizontal()];
    beamHeight = bbox[frame->pointingUp()];
  }

  return Mantid::Kernel::make_unique<RectangularBeamProfile>(
      *frame, instrument.getSource()->getPos(), beamWidth, beamHeight);
}
}
}
