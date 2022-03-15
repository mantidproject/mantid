#include "MantidMDAlgorithms/IntegrateEllipsoids.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/Property.h"
#include "MantidMDAlgorithms/IntegrateEllipsoidsV1.h"
#include "MantidMDAlgorithms/IntegrateEllipsoidsV2.h"

#include <boost/algorithm/string.hpp>

using namespace Mantid::API;
using namespace Mantid::HistogramData;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

namespace Mantid::MDAlgorithms {

DECLARE_ALGORITHM(IntegrateEllipsoids)
//---------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string IntegrateEllipsoids::name() const { return "IntegrateEllipsoids"; }

/// Algorithm's version for identification. @see Algorithm::version
int IntegrateEllipsoids::version() const { return 1; }

<<<<<<< HEAD
/// Only convert to Q-vector.
const std::string Q3D("Q3D");

/// Q-vector is always three dimensional.
const std::size_t DIMS(3);

void IntegrateEllipsoids::qListFromEventWS(IntegrateQLabEvents &integrator, Progress &prog, EventWorkspace_sptr &wksp) {
  auto numSpectra = static_cast<int>(wksp->getNumberHistograms());
  PARALLEL_FOR_IF(Kernel::threadSafe(*wksp))
  for (int i = 0; i < numSpectra; ++i) {
    PARALLEL_START_INTERRUPT_REGION

    // units conversion helper
    UnitsConversionHelper unitConverter;
    unitConverter.initialize(m_targWSDescr, "Momentum");

    // initialize the MD coordinates conversion class
    MDTransfQ3D qConverter;
    qConverter.initialize(m_targWSDescr);

    std::vector<double> buffer(DIMS);
    // get a reference to the event list
    EventList &events = wksp->getSpectrum(i);

    events.switchTo(WEIGHTED_NOTIME);
    events.compressEvents(1e-5, &events);

    // check to see if the event list is empty
    if (events.empty()) {
      prog.report();
      continue; // nothing to do
    }

    // update which pixel is being converted
    std::vector<Mantid::coord_t> locCoord(DIMS, 0.);
    unitConverter.updateConversion(i);
    qConverter.calcYDepCoordinates(locCoord, i);

    // loop over the events
    double signal(1.);  // ignorable garbage
    double errorSq(1.); // ignorable garbage
    const std::vector<WeightedEventNoTime> &raw_events = events.getWeightedEventsNoTime();
    std::vector<std::pair<std::pair<double, double>, V3D>> qList;
    for (const auto &raw_event : raw_events) {
      double val = unitConverter.convertUnits(raw_event.tof());
      qConverter.calcMatrixCoord(val, locCoord, signal, errorSq);
      for (size_t dim = 0; dim < DIMS; ++dim) {
        buffer[dim] = locCoord[dim];
      }
      V3D qVec(buffer[0], buffer[1], buffer[2]);
      qList.emplace_back(std::pair<double, double>(raw_event.m_weight, raw_event.m_errorSquared), qVec);
    } // end of loop over events in list
    PARALLEL_CRITICAL(addEvents) { integrator.addEvents(qList); }

    prog.report();
    PARALLEL_END_INTERRUPT_REGION
  } // end of loop over spectra
  PARALLEL_CHECK_INTERRUPT_REGION
  integrator.populateCellsWithPeaks();
}

void IntegrateEllipsoids::qListFromHistoWS(IntegrateQLabEvents &integrator, Progress &prog, Workspace2D_sptr &wksp) {
  auto numSpectra = static_cast<int>(wksp->getNumberHistograms());
  PARALLEL_FOR_IF(Kernel::threadSafe(*wksp))
  for (int i = 0; i < numSpectra; ++i) {
    PARALLEL_START_INTERRUPT_REGION

    // units conversion helper
    UnitsConversionHelper unitConverter;
    unitConverter.initialize(m_targWSDescr, "Momentum");

    // initialize the MD coordinates conversion class
    MDTransfQ3D qConverter;
    qConverter.initialize(m_targWSDescr);

    std::vector<double> buffer(DIMS);
    // get tof and y values
    const auto &xVals = wksp->points(i);
    const auto &yVals = wksp->y(i);
    const auto &eVals = wksp->e(i);

    // update which pixel is being converted
    std::vector<Mantid::coord_t> locCoord(DIMS, 0.);
    unitConverter.updateConversion(i);
    qConverter.calcYDepCoordinates(locCoord, i);

    // loop over the events
    double signal(1.);  // ignorable garbage
    double errorSq(1.); // ignorable garbage

    SlimEvents qList;
    for (size_t j = 0; j < yVals.size(); ++j) {
      const double &yVal = yVals[j];
      const double &esqVal = eVals[j] * eVals[j]; // error squared (variance)
      if (yVal > 0)                               // TODO, is this condition right?
      {
        double val = unitConverter.convertUnits(xVals[j]);
        qConverter.calcMatrixCoord(val, locCoord, signal, errorSq);
        for (size_t dim = 0; dim < DIMS; ++dim) {
          buffer[dim] = locCoord[dim]; // TODO. Looks un-necessary to me. Can't
                                       // we just add localCoord directly to
                                       // qVec
        }
        V3D qVec(buffer[0], buffer[1], buffer[2]);
        if (std::isnan(qVec[0]) || std::isnan(qVec[1]) || std::isnan(qVec[2]))
          continue;
        // Account for counts in histograms by increasing the qList with the
        // same q-point
        qList.emplace_back(std::pair<double, double>(yVal, esqVal), qVec);
      }
    }
    PARALLEL_CRITICAL(addHisto) { integrator.addEvents(qList); }
    prog.report();
    PARALLEL_END_INTERRUPT_REGION
  } // end of loop over spectra
  PARALLEL_CHECK_INTERRUPT_REGION
  integrator.populateCellsWithPeaks();
}
=======
/// Algorithm's category for identification. @see Algorithm::category
const std::string IntegrateEllipsoids::category() const { return "Crystal\\Integration"; }
//---------------------------------------------------------------------
>>>>>>> added wrapper algo, wip test

void IntegrateEllipsoids::init() {
  auto ws_valid = std::make_shared<CompositeValidator>();
  ws_valid->add<WorkspaceUnitValidator>("TOF");
  ws_valid->add<InstrumentValidator>();
  // the validator which checks if the workspace has axis

  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input, ws_valid),
      "An input MatrixWorkspace with time-of-flight units along "
      "X-axis and defined instrument with defined sample");

  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>("PeaksWorkspace", "", Direction::InOut),
                  "Workspace with Peaks to be integrated. NOTE: The peaks MUST "
                  "be indexed with integer HKL values.");

  std::shared_ptr<BoundedValidator<double>> mustBePositive(new BoundedValidator<double>());
  mustBePositive->setLower(0.0);

  declareProperty("RegionRadius", .35, mustBePositive,
                  "Only events at most this distance from a peak will be "
                  "considered when integrating");

  declareProperty("SpecifySize", false, "If true, use the following for the major axis sizes, else use 3-sigma");

  declareProperty("PeakSize", .18, mustBePositive, "Half-length of major axis for peak ellipsoid");

  declareProperty("BackgroundInnerSize", .18, mustBePositive,
                  "Half-length of major axis for inner ellipsoidal surface of "
                  "background region");

  declareProperty("BackgroundOuterSize", .23, mustBePositive,
                  "Half-length of major axis for outer ellipsoidal surface of "
                  "background region");

  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The output PeaksWorkspace will be a copy of the input PeaksWorkspace "
                  "with the peaks' integrated intensities.");

  declareProperty("CutoffIsigI", EMPTY_DBL(), mustBePositive,
                  "Cuttoff for I/sig(i) when finding mean of half-length of "
                  "major radius in first pass when SpecifySize is false."
                  "Default is no second pass.");

  declareProperty("NumSigmas", 3,
                  "Number of sigmas to add to mean of half-length of "
                  "major radius for second pass when SpecifySize is false.");
  declareProperty("IntegrateInHKL", false, "If true, integrate in HKL space not Q space.");
  declareProperty("IntegrateIfOnEdge", true,
                  "Set to false to not integrate if peak radius is off edge of detector."
                  "Background will be scaled if background radius is off edge.");

  declareProperty("AdaptiveQBackground", false,
                  "Default is false.   If true, "
                  "BackgroundOuterRadius + AdaptiveQMultiplier * **|Q|** and "
                  "BackgroundInnerRadius + AdaptiveQMultiplier * **|Q|**");

  declareProperty("AdaptiveQMultiplier", 0.0,
                  "PeakRadius + AdaptiveQMultiplier * **|Q|** "
                  "so each peak has a "
                  "different integration radius.  Q includes the 2*pi factor.");

  declareProperty("UseOnePercentBackgroundCorrection", true,
                  "If this options is enabled, then the the top 1% of the "
                  "background will be removed"
                  "before the background subtraction.");

  // satellite realted properties
  declareProperty("SatelliteRegionRadius", EMPTY_DBL(), mustBePositive,
                  "Only events at most this distance from a satellite peak will be considered when integration");
  declareProperty("SatellitePeakSize", EMPTY_DBL(), mustBePositive,
                  "Half-length of major axis for satellite peak ellipsoid");
  declareProperty("ShareBackground", false, "Whether to use the same peak background region for satellite peaks.");
  declareProperty(
      "SatelliteBackgroundInnerSize", EMPTY_DBL(), mustBePositive,
      "Half-length of major axis for the inner ellipsoidal surface of background region of the satellite peak");
  declareProperty(
      "SatelliteBackgroundOuterSize", EMPTY_DBL(), mustBePositive,
      "Half-length of major axis for the outer ellipsoidal surface of background region of the satellite peak");
  declareProperty("GetUBFromPeaksWorkspace", false, "If true, UB is taken from peak workspace.");
}

int getIndexCount(PeaksWorkspace_sptr peakWorkspace) {
  int indexCount = 0;
  const int numPeaks = peakWorkspace->getNumberPeaks();
  for (int i = 0; i < numPeaks; ++i) {
    const auto peak = peakWorkspace->getPeak(i);
    if (peak.getHKL().norm2() > 0)
      indexCount += 1;
  }
  return indexCount;
}

void IntegrateEllipsoids::exec() {
  const bool isIntegrateInHKL = getProperty("IntegrateInHKL");
  const bool isGetUBFromPeaksWorkspace = getProperty("GetUBFromPeaksWorkspace");
  const bool shareBackground = getProperty("ShareBackground");

  const PeaksWorkspace_sptr peakWorkspace = getProperty("PeaksWorkspace");

  const int indexCount = getIndexCount(peakWorkspace);

  IAlgorithm_sptr alg;

  // detect which algo to run
  if ((isIntegrateInHKL || isGetUBFromPeaksWorkspace) && indexCount > 0 && !shareBackground) {
    // v1
    alg = createChildAlgorithm("IntegrateEllipsoidsV1", 0.0, 1.0);
  } else {
    // v2
    alg = createChildAlgorithm("IntegrateEllipsoidsV2", 0.0, 1.0);
  }
  alg->initialize();

  // forward properties to algo
  const std::vector<Property *> &props = alg->getProperties();
  for (auto prop : props) {
    if (prop) {
      if (boost::starts_with(prop->type(), "MatrixWorkspace")) {
        MatrixWorkspace_sptr workspace = getProperty(prop->name());
        alg->setProperty(prop->name(), workspace);
      } else if (boost::starts_with(prop->type(), "PeaksWorkspace")) {
        PeaksWorkspace_sptr workspace = getProperty(prop->name());
        alg->setProperty(prop->name(), workspace);
      } else {
        alg->setPropertyValue(prop->name(), getPropertyValue(prop->name()));
      }
    }
  }
  // childAlg->copyPropertiesFrom(*this); TODO look into this
  // run correct algo and return results
  alg->executeAsChildAlg();
  PeaksWorkspace_sptr outputWorkspace = alg->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", outputWorkspace);
}
} // namespace Mantid::MDAlgorithms
