//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAlgorithms/MonteCarloAbsorption.h"
#include "MantidAlgorithms/SampleCorrections/MCAbsorptionStrategy.h"
#include "MantidAlgorithms/SampleCorrections/RectangularBeamProfile.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SampleEnvironment.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"

#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/DeltaEMode.h"
#include "MantidKernel/MersenneTwister.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/VectorHelper.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
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

/// Get ith wavelength point for point data
/// Assumes all checks on sizes have been done before calling
double getWavelengthPointData(int i, const std::vector<double> &lambdas) {
  return lambdas[i];
}

/// Get ith wavelength point for histogram data
double getWavelengthHistogramData(int i, const std::vector<double> &lambdas) {
  return 0.5 * (lambdas[i] + lambdas[i + 1]);
}

struct EFixedProvider {
  explicit EFixedProvider(const ExperimentInfo &expt)
      : m_expt(expt), m_emode(expt.getEMode()), m_value(0.0) {
    if (m_emode == DeltaEMode::Direct) {
      m_value = m_expt.getEFixed();
    }
  }
  inline DeltaEMode::Type emode() const { return m_emode; }
  inline double value(const IDetector_const_sptr &det) const {
    if (m_emode != DeltaEMode::Indirect)
      return m_value;
    else
      return m_expt.getEFixed(det);
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
}

/**
 * Execution code
 */
void MonteCarloAbsorption::exec() {
  const MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  const int nevents = getProperty("EventsPerPoint");
  const int nlambda = getProperty("NumberOfWavelengthPoints");
  const int seed = getProperty("SeedValue");

  auto outputWS =
      doSimulation(*inputWS, static_cast<size_t>(nevents), nlambda, seed);

  setProperty("OutputWorkspace", outputWS);
}

/**
 * Run the simulation over the whole input workspace
 * @param inputWS A reference to the input workspace
 * @param nevents Number of MC events per wavelength point to simulate
 * @param nlambda Number of wavelength points to simulate. The remainder
 * are computed using interpolation
 * @param seed Seed value for the random number generator
 * @return A new workspace containing the correction factors & errors
 */
MatrixWorkspace_sptr
MonteCarloAbsorption::doSimulation(const MatrixWorkspace &inputWS,
                                   size_t nevents, int nlambda, int seed) {
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
  typedef double (*LambdaPointProvider)(int, const std::vector<double> &);
  LambdaPointProvider lambda;
  if (inputWS.isHistogramData()) {
    lambda = &getWavelengthHistogramData;
  } else {
    lambda = &getWavelengthPointData;
  }

  PARALLEL_FOR1(outputWS)
  for (int64_t i = 0; i < nhists; ++i) {
    PARALLEL_START_INTERUPT_REGION

    const auto &xvalues = outputWS->readX(i);
    auto &signal = outputWS->dataY(i);
    auto &errors = outputWS->dataE(i);
    // The input was cloned so clear the errors out
    // Y values are all overwritten later
    std::fill(errors.begin(), errors.end(), 0.0);

    // Final detector position
    IDetector_const_sptr detector;
    try {
      detector = outputWS->getDetector(i);
    } catch (Kernel::Exception::NotFoundError &) {
      continue;
    }
    // Per spectrum values
    const auto &detPos = detector->getPos();
    const double lambdaFixed = toWavelength(efixed.value(detector));
    MersenneTwister rng(seed);

    // Simulation for each requested wavelength point
    for (int j = 0; j < nbins; j += lambdaStepSize) {
      prog.report(reportMsg);
      const double lambdaStep = lambda(j, xvalues);
      double lambdaIn(lambdaStep), lambdaOut(lambdaStep);
      if (efixed.emode() == DeltaEMode::Direct) {
        lambdaIn = lambdaFixed;
      } else if (efixed.emode() == DeltaEMode::Indirect) {
        lambdaOut = lambdaFixed;
      } else {
        // elastic case already initialized
      }
      std::tie(signal[j], std::ignore) =
          strategy.calculate(rng, detPos, lambdaIn, lambdaOut);

      // Ensure we have the last point for the interpolation
      if (lambdaStepSize > 1 && j + lambdaStepSize >= nbins && j + 1 != nbins) {
        j = nbins - lambdaStepSize - 1;
      }
    }

    // Interpolate through points not simulated
    if (lambdaStepSize > 1) {
      Kernel::VectorHelper::linearlyInterpolateY(xvalues, signal,
                                                 lambdaStepSize);
    }

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  return outputWS;
}

MatrixWorkspace_sptr MonteCarloAbsorption::createOutputWorkspace(
    const MatrixWorkspace &inputWS) const {
  MatrixWorkspace_sptr outputWS = inputWS.clone();
  // The algorithm computes the signal values at bin centres so they should
  // be treated as a distribution
  outputWS->isDistribution(true);
  outputWS->setYUnit("");
  outputWS->setYUnitLabel("Attenuation factor");
  return outputWS;
}

/**
 * Create the requested beam profile. Currently hardcoded to a rectangular
 * beam of the same height/width as the sample's bounding box
 * @param instrument A reference to the instrument object
 * @param sample A reference to the sample object
 * @return A new IBeamProfile object
 */
std::unique_ptr<IBeamProfile>
MonteCarloAbsorption::createBeamProfile(const Instrument &instrument,
                                        const Sample &sample) const {
  // This should ultimately come from information set by the user
  const auto frame = instrument.getReferenceFrame();
  const auto bbox = sample.getShape().getBoundingBox().width();

  double beamWidth(bbox[frame->pointingHorizontal()]),
      beamHeight(bbox[frame->pointingUp()]);
  return Mantid::Kernel::make_unique<RectangularBeamProfile>(
      *frame, instrument.getSource()->getPos(), beamWidth, beamHeight);
}
}
}
