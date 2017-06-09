#include "MantidAlgorithms/MonteCarloAbsorption.h"
#include "MantidAlgorithms/InterpolationOption.h"
#include "MantidAlgorithms/SampleCorrections/DetectorGridDefinition.h"
#include "MantidAlgorithms/SampleCorrections/MCAbsorptionStrategy.h"
#include "MantidAlgorithms/SampleCorrections/RectangularBeamProfile.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/DetectorInfo.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
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
#include "MantidKernel/EnabledWhenProperty.h"
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

std::tuple<double, double> extremeWavelengths(const MatrixWorkspace &ws) {
  double currentMin = std::numeric_limits<double>::max();
  double currentMax = std::numeric_limits<double>::lowest();
  if (ws.histogram(0).xMode() == Mantid::HistogramData::Histogram::XMode::BinEdges) {
    for (size_t i = 0; i < ws.getNumberHistograms(); ++i) {
      const auto &xs = ws.x(i);
      const auto x0 = (xs[0] + xs[1]) / 2.0;
      if (x0 < currentMin) currentMin = x0;
      const auto x1 = (xs[xs.size() - 2] + xs[xs.size() - 1]) / 2.0;
      if (x1 > currentMax) currentMax = x1;
    }
  } else {
    for (size_t i = 0; i < ws.getNumberHistograms(); ++i) {
      const auto &xs = ws.x(i);
      const auto x0 = xs.front();
      if (x0 < currentMin) currentMin = x0;
      const auto x1 = xs.back();
      if (x1 > currentMax) currentMax = x1;
    }
  }
  return std::tie(currentMin, currentMax);
}

Mantid::HistogramData::Histogram modelHistogram(const MatrixWorkspace &modelWS, const size_t wavelengthPoints) {
  double minWavelength, maxWavelength;
  std::tie(minWavelength, maxWavelength) = extremeWavelengths(modelWS);
  Mantid::HistogramData::Frequencies ys(wavelengthPoints, 0.0);
  Mantid::HistogramData::FrequencyVariances es(wavelengthPoints, 0.0);
  Mantid::HistogramData::Points ps(wavelengthPoints, 0.0);
  Mantid::HistogramData::Histogram h(ps, ys, es);
  auto &xs = h.mutableX();
  if (wavelengthPoints > 1) {
    const double step = (maxWavelength - minWavelength) / static_cast<double>(wavelengthPoints - 1);
    for (size_t i = 0; i < xs.size(); ++i) {
      xs[i] = minWavelength + step * static_cast<double>(i);
    }
  } else {
    xs.front() = (minWavelength + maxWavelength) / 2.0;
  }
  return h;
}

bool constantEFixed(const EFixedProvider &eFixed, const std::vector<Mantid::detid_t> &detIDs) {
  const auto e = eFixed.value(detIDs[0]);
  for (size_t i = 1; i < detIDs.size(); ++i) {
    if (e != eFixed.value(detIDs[i])) {
      return false;
    }
  }
  return true;
}

Object_sptr makeCubeShape() {
  using namespace Poco::XML;
  const double dimension = 0.05;
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

MatrixWorkspace_uptr createWSWithSimulationInstrument(const MatrixWorkspace &modelWS, const Mantid::Algorithms::DetectorGridDefinition &grid, const size_t wavelengthPoints) {
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
  const size_t numSpectra = grid.numberColumns() * grid.numberRows();
  const auto h = modelHistogram(modelWS, wavelengthPoints);
  auto ws = Mantid::DataObjects::create<Workspace2D>(numSpectra, h);
  auto detShape = makeCubeShape();
  for (size_t col = 0; col < grid.numberColumns(); ++col) {
    const auto lon = grid.longitudeAt(col);
    for (size_t row = 0; row < grid.numberRows(); ++row) {
      const auto lat = grid.latitudeAt(row);
      const size_t index = col * grid.numberRows() + row;
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
  // Add information about EFixed in a proper place.
  EFixedProvider eFixed(modelWS);
  ws->mutableRun().addProperty("deltaE-mode", Mantid::Kernel::DeltaEMode::asString(eFixed.emode()));
  if (eFixed.emode() == Mantid::Kernel::DeltaEMode::Direct) {
    ws->mutableRun().addProperty("Ei", eFixed.value(0));
  } else if (eFixed.emode() == Mantid::Kernel::DeltaEMode::Indirect) {
    const auto &detIDs = modelWS.detectorInfo().detectorIDs();
    if (!constantEFixed(eFixed, detIDs)) {
      throw std::runtime_error("Sparse instrument with variable EFixed not supported.");
    }
    const auto e = eFixed.value(detIDs[0]);
    const auto &sparseDetIDs = ws->detectorInfo().detectorIDs();
    for (size_t i = 0; i < sparseDetIDs.size(); ++i) {
      ws->setEFixed(sparseDetIDs[i], e);
    }
  }
  return MatrixWorkspace_uptr(ws.release());
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

Mantid::HistogramData::Histogram interpolateFromDetectorGrid(const double lat, const double lon, const MatrixWorkspace &ws, const std::array<size_t, 4> &indices) {
  auto h = ws.histogram(0);
  const auto &spectrumInfo = ws.spectrumInfo();
  std::array<double, 4> distances;
  for (size_t i = 0; i < 4; ++i) {
    double detLat, detLong;
   std::tie(detLat, detLong) = geographicalAngles(spectrumInfo.position(indices[i]));
    distances[i] = greatCircleDistance(lat, lon, detLat, detLong);
  }
  const auto weights = inverseDistanceWeights(distances);
  auto weightSum = weights[0];
  h.mutableY() = weights[0] * ws.y(indices[0]);
  for (size_t i = 1; i < 4; ++i) {
    weightSum += weights[i];
    h.mutableY() += weights[i] * ws.y(indices[i]);
  }
  h.mutableY() /= weightSum;
  return h;
}

struct SparseInstrumentOption {
  bool use;
  int latitudinalDets = DEFAULT_LATITUDINAL_DETS;
  int longitudinalDets = DEFAULT_LONGITUDINAL_DETS;
  size_t wavelengthPoints = 2;

  SparseInstrumentOption(const bool use_) : use(use_) {}
};

std::unique_ptr<const Mantid::Algorithms::DetectorGridDefinition> createDetectorGridDefinition(const MatrixWorkspace &modelWS, const SparseInstrumentOption &options) {
  double minLat, maxLat, minLong, maxLong;
  std::tie(minLat, maxLat, minLong, maxLong) = extremeAngles(modelWS);
  return Mantid::Kernel::make_unique<Mantid::Algorithms::DetectorGridDefinition>(minLat, maxLat, static_cast<size_t>(options.latitudinalDets), minLong, maxLong, static_cast<size_t>(options.longitudinalDets));
}

MatrixWorkspace_uptr createSparseWS(const MatrixWorkspace &modelWS, const SparseInstrumentOption &options, const Mantid::Algorithms::DetectorGridDefinition &detGrid) {
  double minWavelength, maxWavelength;
  std::tie(minWavelength, maxWavelength) = extremeWavelengths(modelWS);
  return createWSWithSimulationInstrument(modelWS, detGrid, options.wavelengthPoints);
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
  auto twoOrMore = boost::make_shared<Kernel::BoundedValidator<int>>();
  twoOrMore->setLower(2);
  declareProperty("NumberOfDetectorRows", DEFAULT_LATITUDINAL_DETS, twoOrMore, "Number of detector rows in the detector grid of the sparse instrument.");
  setPropertySettings("NumberOfDetectorRows", Kernel::make_unique<EnabledWhenProperty>("SparseInstrument", ePropertyCriterion::IS_NOT_DEFAULT));
  declareProperty("NumberOfDetectorColumns", DEFAULT_LONGITUDINAL_DETS, twoOrMore, "Number of detector columns in the detector grid of the sparse instrument.");
  setPropertySettings("NumberOfDetectorColumns", Kernel::make_unique<EnabledWhenProperty>("SparseInstrument", ePropertyCriterion::IS_NOT_DEFAULT));
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
                                   const size_t nevents, int nlambda, const int seed,
                                   const InterpolationOption &interpolateOpt,
                                   const bool useSparseInstrument) {
  auto outputWS = createOutputWorkspace(inputWS);
  const auto inputNbins = static_cast<int>(inputWS.blocksize());
  if (isEmpty(nlambda) || nlambda > inputNbins) {
    if (!isEmpty(nlambda)) {
      g_log.warning() << "The requested number of wavelength points is larger "
                         "than the spectra size. "
                         "Defaulting to spectra size.\n";
    }
    nlambda = inputNbins;
  }
  SparseInstrumentOption sparseInstrumentOpt(useSparseInstrument);
  std::unique_ptr<const DetectorGridDefinition> detGrid;
  MatrixWorkspace_uptr sparseWS;
  if (sparseInstrumentOpt.use) {
    sparseInstrumentOpt.latitudinalDets = getProperty("NumberOfDetectorRows");
    sparseInstrumentOpt.longitudinalDets = getProperty("NumberOfDetectorColumns");
    sparseInstrumentOpt.wavelengthPoints = nlambda;
    detGrid = createDetectorGridDefinition(inputWS, sparseInstrumentOpt);
    sparseWS = createSparseWS(inputWS, sparseInstrumentOpt, *detGrid);
  }
  MatrixWorkspace &simulationWS = sparseInstrumentOpt.use ? *sparseWS : *outputWS;
  const MatrixWorkspace &instrumentWS = sparseInstrumentOpt.use ? simulationWS : inputWS;
  // Cache information about the workspace that will be used repeatedly
  auto instrument = instrumentWS.getInstrument();
  const int64_t nhists = static_cast<int64_t>(instrumentWS.getNumberHistograms());
  const int nbins = static_cast<int>(simulationWS.blocksize());

  EFixedProvider efixed(instrumentWS);
  auto beamProfile = createBeamProfile(*instrument, inputWS.sample());

  // Configure progress
  const int lambdaStepSize = nbins / nlambda;
  Progress prog(this, 0.0, 1.0, nhists * nbins / lambdaStepSize);
  prog.setNotifyStep(0.01);
  const std::string reportMsg = "Computing corrections";

  // Configure strategy
  MCAbsorptionStrategy strategy(*beamProfile, inputWS.sample(), nevents);

  const auto &spectrumInfo = simulationWS.spectrumInfo();

  PARALLEL_FOR_IF(Kernel::threadSafe(simulationWS))
  for (int64_t i = 0; i < nhists; ++i) {
    PARALLEL_START_INTERUPT_REGION

    auto &outE = simulationWS.mutableE(i);
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

    auto &outY = simulationWS.mutableY(i);
    const auto lambdas = simulationWS.points(i);
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
    if (!sparseInstrumentOpt.use && lambdaStepSize > 1) {
        auto histnew = simulationWS.histogram(i);
        if (lambdaStepSize < nbins) {
          interpolateOpt.applyInplace(histnew, lambdaStepSize);
        } else {
          std::fill(histnew.mutableY().begin() + 1, histnew.mutableY().end(), histnew.y()[0]);
        }

        outputWS->setHistogram(i, histnew);
    }

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  if (sparseInstrumentOpt.use) {
    interpolateFromSparse(*outputWS, simulationWS, interpolateOpt, *detGrid);
  }

  //return std::move(sparseInstrumentOpt.use ? sparseWS : outputWS); // TODO remove this line
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
      *frame, source->getPos(), beamWidth, beamHeight);
}

void MonteCarloAbsorption::interpolateFromSparse(MatrixWorkspace &targetWS, const MatrixWorkspace &sparseWS, const Mantid::Algorithms::InterpolationOption &interpOpt, const DetectorGridDefinition &detGrid) {
  const auto &spectrumInfo = targetWS.spectrumInfo();
  PARALLEL_FOR_IF(Kernel::threadSafe(targetWS, sparseWS))
  for (int64_t i = 0; i < static_cast<decltype(i)>(spectrumInfo.size()); ++i) {
    PARALLEL_START_INTERUPT_REGION
    double lat, lon;
    std::tie(lat, lon) = geographicalAngles(spectrumInfo.position(i));
    const auto nearestIndices = detGrid.nearestNeighbourIndices(lat, lon);
    const auto spatiallyInterpHisto = interpolateFromDetectorGrid(lat, lon, sparseWS, nearestIndices);
    if (spatiallyInterpHisto.size() > 1) {
      auto targetHisto = targetWS.histogram(i);
      interpOpt.applyInPlace(spatiallyInterpHisto, targetHisto);
      targetWS.setHistogram(i, targetHisto);
    } else {
      targetWS.mutableY(i) = spatiallyInterpHisto.y().front();
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

}
}
