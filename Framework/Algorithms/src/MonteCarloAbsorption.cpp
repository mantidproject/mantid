#include "MantidAlgorithms/MonteCarloAbsorption.h"
#include "MantidAlgorithms/InterpolationOption.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Run.h"
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
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/DeltaEMode.h"
#include "MantidKernel/MersenneTwister.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/VectorHelper.h"

#include "MantidHistogramData/HistogramX.h"
#include "MantidHistogramData/Interpolate.h"

#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Document.h>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using Mantid::HistogramData::Histogram;
using Mantid::HistogramData::HistogramX;
using Mantid::HistogramData::Points;
using Mantid::HistogramData::interpolateLinearInplace;
using Mantid::DataObjects::Workspace2D;
namespace PhysicalConstants = Mantid::PhysicalConstants;

/// @cond
namespace {

constexpr int DEFAULT_NEVENTS = 300;
constexpr int DEFAULT_SEED = 123456789;
constexpr int DEFAULT_LATITUDINAL_DETS = 4;
constexpr int DEFAULT_LONGITUDINAL_DETS = 10;

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

std::pair<double, double> geographicalAngles(const V3D &p) {
  const double lat = std::atan2(p.Y(), std::hypot(p.X() , p.Z()));
  const double lon = std::atan2(p.X(), p.Z());
  return std::pair<double, double>(lat, lon);
}

std::tuple<double, double, double, double> extremeAngles(const MatrixWorkspace &ws) {
  const auto &spectrumInfo = ws.spectrumInfo();
  double minLat = std::numeric_limits<double>::max();
  double maxLat = std::numeric_limits<double>::lowest();
  double minLong = std::numeric_limits<double>::max();
  double maxLong = std::numeric_limits<double>::lowest();
  for (size_t i = 0; i < ws.getNumberHistograms(); ++i) {
    double lat, lon;
    std::tie(lat, lon) = geographicalAngles(spectrumInfo.position(i));
    if (lat < minLat) {
      minLat = lat;
    } else if (lat > maxLat) {
      maxLat = lat;
    }
    if (lon < minLong) {
      minLong = lon;
    } else if (lon > maxLong) {
      maxLong = lon;
    }
  }
  return std::tie(minLat, maxLat, minLong, maxLong);
}

std::tuple<std::set<double>, std::set<double>> latitudeLongitudeGrid(const double minLat, const double maxLat, const size_t latPoints, const double minLong, const double maxLong, const size_t longPoints) {
  const double latStep = (maxLat - minLat) / static_cast<double>(latPoints - 1);
  std::set<double> lats;
  for (size_t i = 0; i < latPoints; ++i) {
    lats.emplace_hint(lats.cend(), minLat + static_cast<double>(i) * latStep);
  }
  const double longStep = (maxLong - minLong) / static_cast<double>(longPoints - 1);
  std::set<double> longs;
  for (size_t i = 0; i < longPoints; ++i) {
    longs.emplace_hint(longs.cend(), minLong + static_cast<double>(i) * longStep);
  }
  return std::tie(lats, longs);
}

Object_sptr makeCubeShape() {
  using namespace Poco::XML;
  const double dimension = 0.02;
  AutoPtr<Document> shapeDescription = new Document;
  AutoPtr<Element> typeElement = shapeDescription->createElement("type");
  typeElement->setAttribute("name", "detector");
  AutoPtr<Element> shapeElement = shapeDescription->createElement("cuboid");
  shapeElement->setAttribute("id", "cube");
  const std::string posCoord = std::to_string(dimension / 2);
  const std::string negCoord = std::to_string(-dimension / 2);
  AutoPtr<Element> element =
      shapeDescription->createElement("left-front-bottom-point");
  element->setAttribute("x", negCoord);
  element->setAttribute("y", negCoord);
  element->setAttribute("z", posCoord);
  shapeElement->appendChild(element);
  element = shapeDescription->createElement("left-front-top-point");
  element->setAttribute("x", negCoord);
  element->setAttribute("y", posCoord);
  element->setAttribute("z", posCoord);
  shapeElement->appendChild(element);
  element = shapeDescription->createElement("left-back-bottom-point");
  element->setAttribute("x", negCoord);
  element->setAttribute("y", negCoord);
  element->setAttribute("z", negCoord);
  shapeElement->appendChild(element);
  element = shapeDescription->createElement("right-front-bottom-point");
  element->setAttribute("x", posCoord);
  element->setAttribute("y", negCoord);
  element->setAttribute("z", posCoord);
  shapeElement->appendChild(element);
  typeElement->appendChild(shapeElement);
  AutoPtr<Element> algebraElement =
      shapeDescription->createElement("algebra");
  algebraElement->setAttribute("val", "cube");
  typeElement->appendChild(algebraElement);
  ShapeFactory shapeFactory;
  return shapeFactory.createShape(typeElement);
}

MatrixWorkspace_uptr createWSWithSimulationInstrument(const MatrixWorkspace &modelWS, const std::set<double> &lats, const std::set<double> &longs) {
  auto instrument = boost::make_shared<Instrument>("MC_simulation_instrument");
  instrument->setReferenceFrame(
      boost::make_shared<ReferenceFrame>(Y, Z, Right, ""));
  const V3D samplePos{0.0, 0.0, 0.0};
  auto sample = Mantid::Kernel::make_unique<ObjComponent>("sample", nullptr, instrument.get());
  sample->setPos(samplePos);
  instrument->add(sample.get());
  instrument->markAsSamplePos(sample.release());
  const double R = 1.0;
  const V3D sourcePos{0.0, 0.0, -2.0 * R};
  auto source = Mantid::Kernel::make_unique<ObjComponent>("source", nullptr, instrument.get());
  source->setPos(sourcePos);
  instrument->add(source.get());
  instrument->markAsSource(source.release());
  const size_t numSpectra = lats.size() * longs.size();
  auto ws = Mantid::DataObjects::create<Workspace2D>(numSpectra, modelWS.histogram(0));
  auto detShape = makeCubeShape();
  size_t index = 0;
  for (const auto lat : lats) {
    for (const auto lon : longs) {
      const int detID = static_cast<int>(index);
      std::ostringstream detName;
      detName << "det-" << detID;
      auto det = Mantid::Kernel::make_unique<Detector>(detName.str(), detID, detShape, instrument.get());
      const double x = R * std::sin(lon) * std::cos(lat);
      const double y = R * std::sin(lat);
      const double z = R * std::cos(lon) * std::cos(lat);
      det->setPos(x, y, z);
      ws->getSpectrum(index).setDetectorID(detID);
      instrument->add(det.get());
      instrument->markAsDetector(det.release());
      ++index;
    }
  }
  ws->setInstrument(instrument);
  auto &paramMap = ws->instrumentParameters();
  auto parametrizedInstrument = ws->getInstrument();
  const auto modelSource = modelWS.getInstrument()->getSource();
  const auto beamWidthParam = modelSource->getNumberParameter("beam-width");
  const auto beamHeightParam = modelSource->getNumberParameter("beam-height");
  if (beamWidthParam.size() == 1 && beamHeightParam.size() == 1) {
    auto parametrizedSource = parametrizedInstrument->getSource();
    paramMap.add("double", parametrizedSource.get(), "beam-width", beamWidthParam[0]);
    paramMap.add("double", parametrizedSource.get(), "beam-height", beamHeightParam[0]);
  }
  EFixedProvider eFixed(modelWS);
  ws->mutableRun().addProperty("deltaE-mode", Mantid::Kernel::DeltaEMode::asString(eFixed.emode()));
  if (eFixed.emode() == Mantid::Kernel::DeltaEMode::Direct) {
    ws->mutableRun().addProperty("Ei", eFixed.value(0));
  } else if (eFixed.emode() == Mantid::Kernel::DeltaEMode::Indirect) {
    throw std::runtime_error("Sparse instrument with indirect mode not yet supported.");
  }
  return MatrixWorkspace_uptr(ws.release());
}

struct SparseInstrumentOption {
  bool use;
  int latitudinalDets = DEFAULT_LATITUDINAL_DETS;
  int longitudinalDets = DEFAULT_LONGITUDINAL_DETS;

  SparseInstrumentOption(const bool use_) : use(use_) {}

  MatrixWorkspace_uptr chooseSimulationWS(MatrixWorkspace_uptr nonSparse) const;
  const MatrixWorkspace &chooseInstrumentWS(const MatrixWorkspace_uptr &sparse, const MatrixWorkspace &nonSparse) const;
};

MatrixWorkspace_uptr SparseInstrumentOption::chooseSimulationWS(MatrixWorkspace_uptr nonSparse) const {
  if (!use) {
    return nonSparse;
  }
  double minLat, maxLat, minLong, maxLong;
  std::tie(minLat, maxLat, minLong, maxLong) = extremeAngles(*nonSparse);
  std::set<double> lats, longs;
  std::tie(lats, longs) = latitudeLongitudeGrid(minLat, maxLat, latitudinalDets, minLong, maxLong, longitudinalDets);
  return createWSWithSimulationInstrument(*nonSparse, lats, longs);
}

const MatrixWorkspace &SparseInstrumentOption::chooseInstrumentWS(const MatrixWorkspace_uptr &sparse, const MatrixWorkspace &nonSparse) const {
  return use ? *sparse : nonSparse;
}

std::array<size_t, 4> nearestNeighbourIndices(const double lat, const double lon, const std::set<double> &lats, const std::set<double> &longs) {
  if (lats.upper_bound(lat) == lats.cbegin() || lats.upper_bound(lat) == lats.cend() || longs.upper_bound(lon) == longs.cbegin() || longs.upper_bound(lon) == longs.cend()) {
    throw std::runtime_error("BUG: Simulation instrument was to small to accommodate all detectors.");
  }
  const auto lowerLatBound = std::lower_bound(lats.cbegin(), lats.cend(), lat);
  auto latIndex = static_cast<size_t>(std::distance(lats.cbegin(), lowerLatBound));
  if (latIndex == 0) {
    latIndex = 1;
  } else if (latIndex == lats.size()) {
    --latIndex;
  }
  const auto lowerLongBound = std::lower_bound(longs.cbegin(), longs.cend(), lon);
  auto longIndex = static_cast<size_t>(std::distance(longs.cbegin(), lowerLongBound));
  if (longIndex == 0) {
    longIndex = 1;
  } else if (longIndex == longs.size()) {
    --longIndex;
  }
  std::array<size_t, 4> ids;
  ids[0] = (latIndex - 1) * lats.size() + longIndex - 1;
  ids[1] = ids[0] + 1;
  ids[2] = ids[0] + lats.size();
  ids[3] = ids[2] + 1;
  return ids;
}

double greatCircleDistance(const double lat1, const double long1, const double lat2, const double long2) {
  const double latD = std::sin((lat2 - lat1) / 2.0);
  const double longD = std::sin((long2 - long1) / 2.0);
  const double S = latD * latD + std::cos(lat1) * std::cos(lat2) * longD * longD;
  return 2.0 * std::asin(std::sqrt(S));
}

std::array<double, 4> inverseDistanceWeights(const std::array<double, 4> &distances) {
  std::array<double, 4> weights;
  for (size_t i = 0; i < weights.size(); ++i) {
    if (distances[i] == 0.0) {
      weights.fill(0.0);
      weights[i] = 1.0;
      return weights;
    }
    weights[i] = 1.0 / distances[i] / distances[i];
  }
  return weights;
}

Mantid::HistogramData::HistogramY interpolateY(const double lat, const double lon, const MatrixWorkspace &ws, const std::array<size_t, 4> &indices) {
  const auto &spectrumInfo = ws.spectrumInfo();
  std::array<double, 4> distances;
  for (size_t i = 0; i < 4; ++i) {
    double detLat, detLong;
    std::tie(detLat, detLong) = geographicalAngles(spectrumInfo.position(indices[i]));
    distances[i] = greatCircleDistance(lat, lon, detLat, detLong);
  }
  const auto weights = inverseDistanceWeights(distances);
  auto weightSum = weights[0];
  auto ys = weights[0] * ws.y(indices[0]);
  for (size_t i = 1; i < 4; ++i) {
    weightSum += weights[i];
    ys += weights[i] * ws.y(indices[i]);
  }
  ys /= weightSum;
  return ys;
}

MatrixWorkspace_uptr createInterpolatedWS(MatrixWorkspace &ws, MatrixWorkspace &simulationWS, const std::set<double> &lats, const std::set<double> &longs) {
  auto interpWS = Mantid::DataObjects::create<Workspace2D>(ws, simulationWS.histogram(0));
  const auto &spectrumInfo = interpWS->spectrumInfo();
  for (size_t i = 0; i < interpWS->getNumberHistograms(); ++i) {
    double lat, lon;
    std::tie(lat, lon) = geographicalAngles(spectrumInfo.position(i));
    const auto nearestIndices = nearestNeighbourIndices(lat, lon, lats, longs);
    interpWS->mutableY(i) = interpolateY(lat, lon, simulationWS, nearestIndices);
  }
  return MatrixWorkspace_uptr(interpWS.release());
}

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
  declareProperty("SparseInstrument", false, "Enable simulation on special instrument with a sparse grid of detectors interpolating the results to the real instrument.");
  auto sparseDetectorCount = boost::make_shared<Kernel::BoundedValidator<int>>();
  positiveInt->setLower(2);
  declareProperty("DetectorRows", DEFAULT_LATITUDINAL_DETS, sparseDetectorCount, "Number of detector rows in the detector grid.");
  declareProperty("DetectorColumns", DEFAULT_LONGITUDINAL_DETS, sparseDetectorCount, "Number of detector columns in the detector grid.");
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
  const bool useSparseInstrument = getProperty("SparseInstrument");
  auto outputWS = doSimulation(*inputWS, static_cast<size_t>(nevents), nlambda,
                               seed, interpolateOpt, useSparseInstrument);

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
                                   const InterpolationOption &interpolateOpt,
                                   const bool useSparseInstrument) {
  auto outputWS = createOutputWorkspace(inputWS);
  SparseInstrumentOption sparseInstrumentOpt(useSparseInstrument);
  if (sparseInstrumentOpt.use) {
    sparseInstrumentOpt.latitudinalDets = getProperty("DetectorRows");
    sparseInstrumentOpt.longitudinalDets = getProperty("DetectorColumns");
  }
  MatrixWorkspace_uptr simulationWS{sparseInstrumentOpt.chooseSimulationWS(std::move(outputWS))};
  const MatrixWorkspace &instrumentWS = sparseInstrumentOpt.chooseInstrumentWS(simulationWS, inputWS);
  // Cache information about the workspace that will be used repeatedly
  auto instrument = instrumentWS.getInstrument();
  const int64_t nhists = static_cast<int64_t>(instrumentWS.getNumberHistograms());
  const int nbins = static_cast<int>(instrumentWS.blocksize());
  if (isEmpty(nlambda) || nlambda > nbins) {
    if (!isEmpty(nlambda)) {
      g_log.warning() << "The requested number of wavelength points is larger "
                         "than the spectra size. "
                         "Defaulting to spectra size.\n";
    }
    nlambda = nbins;
  }

  EFixedProvider efixed(instrumentWS);
  auto beamProfile = createBeamProfile(*instrument, inputWS.sample());

  // Configure progress
  const int lambdaStepSize = nbins / nlambda;
  Progress prog(this, 0.0, 1.0, nhists * nbins / lambdaStepSize);
  prog.setNotifyStep(0.01);
  const std::string reportMsg = "Computing corrections";

  // Configure strategy
  MCAbsorptionStrategy strategy(*beamProfile, inputWS.sample(), nevents);

  const auto &spectrumInfo = simulationWS->spectrumInfo();

  PARALLEL_FOR_IF(Kernel::threadSafe(*simulationWS))
  for (int64_t i = 0; i < nhists; ++i) {
    PARALLEL_START_INTERUPT_REGION

    auto &outE = simulationWS->mutableE(i);
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

    auto &outY = simulationWS->mutableY(i);
    const auto lambdas = simulationWS->points(i);
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
      auto histnew = simulationWS->histogram(i);
      interpolateOpt.applyInplace(histnew, lambdaStepSize);
      simulationWS->setHistogram(i, histnew);
    }

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  return simulationWS;
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
      *frame, source->getPos(), beamWidth, beamHeight);
}
}
}
