// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/IntegrateEllipsoidsTwoStep.h"

#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/NearestNeighbours.h"

#include "MantidMDAlgorithms/Integrate3DEvents.h"
#include "MantidMDAlgorithms/MDTransfFactory.h"
#include "MantidMDAlgorithms/MDTransfQ3D.h"
#include "MantidMDAlgorithms/UnitsConversionHelper.h"

#include <boost/math/special_functions/round.hpp>
#include <cmath>
#include <string>
#include <tuple>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(IntegrateEllipsoidsTwoStep)

//---------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string IntegrateEllipsoidsTwoStep::name() const {
  return "IntegrateEllipsoidsTwoStep";
}

/// Algorithm's version for identification. @see Algorithm::version
int IntegrateEllipsoidsTwoStep::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string IntegrateEllipsoidsTwoStep::category() const {
  return "Crystal\\Integration";
}

void IntegrateEllipsoidsTwoStep::init() {
  auto ws_valid = boost::make_shared<CompositeValidator>();
  ws_valid->add<InstrumentValidator>();

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input, ws_valid),
                  "An input MatrixWorkspace with time-of-flight units along "
                  "X-axis and defined instrument with defined sample");

  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "PeaksWorkspace", "", Direction::InOut),
                  "Workspace with peaks to be integrated");

  declareProperty("RegionRadius", .35, mustBePositive,
                  "Only events at most this distance from a peak will be "
                  "considered when integrating");

  declareProperty(
      "SpecifySize", false,
      "If true, use the following for the major axis sizes, else use 3-sigma");

  declareProperty("PeakSize", .18, mustBePositive,
                  "Half-length of major axis for peak ellipsoid");

  declareProperty("BackgroundInnerSize", .18, mustBePositive,
                  "Half-length of major axis for inner ellipsoidal surface of "
                  "background region");

  declareProperty("BackgroundOuterSize", .23, mustBePositive,
                  "Half-length of major axis for outer ellipsoidal surface of "
                  "background region");

  declareProperty("IntegrateInHKL", false,
                  "If true, integrate in HKL space not Q space.");
  declareProperty(
      "IntegrateIfOnEdge", true,
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

  declareProperty("WeakPeakThreshold", 1.0, mustBePositive,
                  "Intensity threshold use to classify a peak as weak.");

  declareProperty("UseOnePercentBackgroundCorrection", true,
                  "If this options is enabled, then the the top 1% of the "
                  "background will be removed"
                  "before the background subtraction.");

  declareProperty(
      std::make_unique<WorkspaceProperty<PeaksWorkspace>>("OutputWorkspace", "",
                                                          Direction::Output),
      "The output PeaksWorkspace will be a copy of the input PeaksWorkspace "
      "with the peaks' integrated intensities.");
}

void IntegrateEllipsoidsTwoStep::exec() {
  PeaksWorkspace_sptr input_peak_ws = getProperty("PeaksWorkspace");
  MatrixWorkspace_sptr input_ws = getProperty("InputWorkspace");
  EventWorkspace_sptr eventWS =
      boost::dynamic_pointer_cast<EventWorkspace>(input_ws);

  Workspace2D_sptr histoWS = boost::dynamic_pointer_cast<Workspace2D>(input_ws);
  if (!eventWS && !histoWS) {
    throw std::runtime_error("IntegrateEllipsoids needs either a "
                             "EventWorkspace or Workspace2D as input.");
  }

  const double weakPeakThreshold = getProperty("WeakPeakThreshold");

  // validation of inputs
  if (!input_peak_ws) {
    throw std::runtime_error("Could not read the Peaks Workspace");
  }

  if (!input_ws) {
    throw std::runtime_error("Could not read the Input Workspace");
  }

  PeaksWorkspace_sptr peak_ws = getProperty("OutputWorkspace");
  if (peak_ws != input_peak_ws) {
    peak_ws = input_peak_ws->clone();
  }

  Progress prog(this, 0.5, 1.0, input_ws->getNumberHistograms());

  std::vector<Peak> &peaks = peak_ws->getPeaks();
  size_t n_peaks = peak_ws->getNumberPeaks();
  size_t indexed_count = 0;
  std::vector<V3D> peak_q_list;
  std::vector<V3D> hkl_vectors;
  for (size_t i = 0; i < n_peaks; i++) // Note: we skip un-indexed peaks
  {
    V3D hkl(peaks[i].getH(), peaks[i].getK(), peaks[i].getL());
    if (Geometry::IndexingUtils::ValidIndex(hkl, 1.0)) // use tolerance == 1 to
                                                       // just check for (0,0,0)
    {
      peak_q_list.emplace_back(peaks[i].getQLabFrame());
      V3D miller_ind(static_cast<double>(boost::math::iround<double>(hkl[0])),
                     static_cast<double>(boost::math::iround<double>(hkl[1])),
                     static_cast<double>(boost::math::iround<double>(hkl[2])));
      hkl_vectors.push_back(miller_ind);
      indexed_count++;
    }
  }

  if (indexed_count < 3) {
    throw std::runtime_error(
        "At least three linearly independent indexed peaks are needed.");
  }
  // Get UB using indexed peaks and
  // lab-Q vectors
  Matrix<double> UB(3, 3, false);
  Geometry::IndexingUtils::Optimize_UB(UB, hkl_vectors, peak_q_list);
  Matrix<double> UBinv(UB);
  UBinv.Invert();
  UBinv *= (1.0 / (2.0 * M_PI));

  std::vector<std::pair<double, V3D>> qList;
  for (size_t i = 0; i < n_peaks; i++) {
    qList.emplace_back(1., V3D(peaks[i].getQLabFrame()));
  }

  const bool integrateEdge = getProperty("IntegrateIfOnEdge");
  if (!integrateEdge) {
    // This only fails in the unit tests which say that MaskBTP is not
    // registered
    try {
      runMaskDetectors(input_peak_ws, "Tube", "edges");
      runMaskDetectors(input_peak_ws, "Pixel", "edges");
    } catch (...) {
      g_log.error("Can't execute MaskBTP algorithm for this instrument to set "
                  "edge for IntegrateIfOnEdge option");
    }
    calculateE1(
        input_peak_ws->detectorInfo()); // fill E1Vec for use in detectorQ
  }

  const bool integrateInHKL = getProperty("IntegrateInHKL");
  bool useOnePercentBackgroundCorrection =
      getProperty("UseOnePercentBackgroundCorrection");
  Integrate3DEvents integrator(qList, UBinv, getProperty("RegionRadius"),
                               useOnePercentBackgroundCorrection);

  if (eventWS) {
    // process as EventWorkspace
    qListFromEventWS(integrator, prog, eventWS, UBinv, integrateInHKL);
  } else {
    // process as Workspace2D
    qListFromHistoWS(integrator, prog, histoWS, UBinv, integrateInHKL);
  }

  std::vector<std::pair<int, V3D>> weakPeaks, strongPeaks;

  // Compute signal to noise ratio for all peaks
  for (int index = 0; static_cast<size_t>(index) < qList.size(); ++index) {
    const auto &item = qList[index];
    const auto center = item.second;
    IntegrationParameters params = makeIntegrationParameters(center);
    auto sig2noise = integrator.estimateSignalToNoiseRatio(params, center);

    auto &peak = peak_ws->getPeak(index);
    peak.setIntensity(0);
    peak.setSigmaIntensity(0);

    const auto result = std::make_pair(index, center);
    if (sig2noise < weakPeakThreshold) {
      g_log.notice() << "Peak " << peak.getHKL() << " with Q = " << center
                     << " is a weak peak with signal to noise " << sig2noise
                     << "\n";
      weakPeaks.push_back(result);
    } else {
      g_log.notice() << "Peak " << peak.getHKL() << " with Q = " << center
                     << " is a strong peak with signal to noise " << sig2noise
                     << "\n";
      strongPeaks.push_back(result);
    }
  }

  std::vector<std::pair<boost::shared_ptr<const Geometry::PeakShape>,
                        std::tuple<double, double, double>>>
      shapeLibrary;

  // Integrate strong peaks
  for (const auto &item : strongPeaks) {
    const auto index = item.first;
    const auto &q = item.second;
    double inti, sigi;

    IntegrationParameters params = makeIntegrationParameters(q);
    const auto result = integrator.integrateStrongPeak(params, q, inti, sigi);
    shapeLibrary.push_back(result);

    auto &peak = peak_ws->getPeak(index);
    peak.setIntensity(inti);
    peak.setSigmaIntensity(sigi);
    peak.setPeakShape(std::get<0>(result));
  }

  std::vector<Eigen::Vector3d> points;
  std::transform(strongPeaks.begin(), strongPeaks.end(),
                 std::back_inserter(points),
                 [&](const std::pair<int, V3D> &item) {
                   const auto q = item.second;
                   return Eigen::Vector3d(q[0], q[1], q[2]);
                 });

  if (points.empty())
    throw std::runtime_error("Cannot integrate peaks when all peaks are below "
                             "the signal to noise ratio.");

  NearestNeighbours<3> kdTree(points);

  // Integrate weak peaks
  for (const auto &item : weakPeaks) {
    double inti, sigi;
    const auto index = item.first;
    const auto &q = item.second;

    const auto result = kdTree.findNearest(Eigen::Vector3d(q[0], q[1], q[2]));
    const auto strongIndex = static_cast<int>(std::get<1>(result[0]));

    auto &peak = peak_ws->getPeak(index);
    auto &strongPeak = peak_ws->getPeak(strongIndex);

    g_log.notice() << "Integrating weak peak " << peak.getHKL()
                   << " using strong peak " << strongPeak.getHKL() << "\n";

    const auto libShape = shapeLibrary[static_cast<int>(strongIndex)];
    const auto shape =
        boost::dynamic_pointer_cast<const PeakShapeEllipsoid>(libShape.first);
    const auto frac = std::get<0>(libShape.second);

    g_log.notice() << "Weak peak will be adjusted by " << frac << "\n";
    IntegrationParameters params =
        makeIntegrationParameters(strongPeak.getQLabFrame());
    const auto weakShape = integrator.integrateWeakPeak(
        params, shape, libShape.second, q, inti, sigi);

    peak.setIntensity(inti);
    peak.setSigmaIntensity(sigi);
    peak.setPeakShape(weakShape);
  }

  // This flag is used by the PeaksWorkspace to evaluate whether it has been
  // integrated.
  peak_ws->mutableRun().addProperty("PeaksIntegrated", 1, true);
  setProperty("OutputWorkspace", peak_ws);
}

IntegrationParameters
IntegrateEllipsoidsTwoStep::makeIntegrationParameters(const V3D &peak_q) const {
  IntegrationParameters params;
  params.peakRadius = getProperty("PeakSize");
  params.backgroundInnerRadius = getProperty("BackgroundInnerSize");
  params.backgroundOuterRadius = getProperty("BackgroundOuterSize");
  params.regionRadius = getProperty("RegionRadius");
  params.specifySize = getProperty("SpecifySize");
  params.E1Vectors = E1Vec;

  const bool adaptiveQBackground = getProperty("AdaptiveQBackground");
  const double adaptiveQMultiplier = getProperty("AdaptiveQMultiplier");
  const double adaptiveQBackgroundMultiplier =
      (adaptiveQBackground) ? adaptiveQMultiplier : 0.0;

  // modulus of Q
  const double lenQpeak = peak_q.norm();
  // change params to support adaptive Q
  params.peakRadius = adaptiveQMultiplier * lenQpeak + params.peakRadius;
  params.backgroundInnerRadius =
      adaptiveQBackgroundMultiplier * lenQpeak + params.backgroundInnerRadius;
  params.backgroundOuterRadius =
      adaptiveQBackgroundMultiplier * lenQpeak + params.backgroundOuterRadius;
  return params;
}

void IntegrateEllipsoidsTwoStep::qListFromEventWS(Integrate3DEvents &integrator,
                                                  Progress &prog,
                                                  EventWorkspace_sptr &wksp,
                                                  DblMatrix const &UBinv,
                                                  bool hkl_integ) {
  // loop through the eventlists

  const std::string ELASTIC("Elastic");
  /// Only convert to Q-vector.
  const std::string Q3D("Q3D");
  const std::size_t DIMS(3);

  MDWSDescription m_targWSDescr;
  m_targWSDescr.setMinMax(std::vector<double>(3, -2000.),
                          std::vector<double>(3, 2000.));
  m_targWSDescr.buildFromMatrixWS(wksp, Q3D, ELASTIC);
  m_targWSDescr.setLorentsCorr(false);

  // generate the detectors table
  Mantid::API::Algorithm_sptr childAlg = createChildAlgorithm(
      "PreprocessDetectorsToMD", 0.,
      .5); // HACK. soft dependency on non-dependent package.
  childAlg->setProperty("InputWorkspace", wksp);
  childAlg->executeAsChildAlg();

  DataObjects::TableWorkspace_sptr table =
      childAlg->getProperty("OutputWorkspace");
  if (!table)
    throw(std::runtime_error(
        "Can not retrieve results of \"PreprocessDetectorsToMD\""));

  m_targWSDescr.m_PreprDetTable = table;

  auto numSpectra = static_cast<int>(wksp->getNumberHistograms());
  PARALLEL_FOR_IF(Kernel::threadSafe(*wksp))
  for (int i = 0; i < numSpectra; ++i) {
    PARALLEL_START_INTERUPT_REGION

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
    const std::vector<WeightedEventNoTime> &raw_events =
        events.getWeightedEventsNoTime();
    std::vector<std::pair<double, V3D>> qList;
    for (const auto &raw_event : raw_events) {
      double val = unitConverter.convertUnits(raw_event.tof());
      qConverter.calcMatrixCoord(val, locCoord, signal, errorSq);
      for (size_t dim = 0; dim < DIMS; ++dim) {
        buffer[dim] = locCoord[dim];
      }
      V3D qVec(buffer[0], buffer[1], buffer[2]);
      if (hkl_integ)
        qVec = UBinv * qVec;
      qList.emplace_back(raw_event.m_weight, qVec);
    } // end of loop over events in list
    PARALLEL_CRITICAL(addEvents) { integrator.addEvents(qList, hkl_integ); }

    prog.report();
    PARALLEL_END_INTERUPT_REGION
  } // end of loop over spectra
  PARALLEL_CHECK_INTERUPT_REGION
}

/**
 * @brief qListFromHistoWS creates qlist from input workspaces of type
 * Workspace2D
 * @param integrator : itegrator object on which qlists are accumulated
 * @param prog : progress object
 * @param wksp : input Workspace2D
 * @param UBinv : inverse of UB matrix
 * @param hkl_integ ; boolean for integrating in HKL space
 */
void IntegrateEllipsoidsTwoStep::qListFromHistoWS(Integrate3DEvents &integrator,
                                                  Progress &prog,
                                                  Workspace2D_sptr &wksp,
                                                  DblMatrix const &UBinv,
                                                  bool hkl_integ) {

  // loop through the eventlists
  const std::string ELASTIC("Elastic");
  /// Only convert to Q-vector.
  const std::string Q3D("Q3D");
  const std::size_t DIMS(3);

  MDWSDescription m_targWSDescr;
  m_targWSDescr.setMinMax(std::vector<double>(3, -2000.),
                          std::vector<double>(3, 2000.));
  m_targWSDescr.buildFromMatrixWS(wksp, Q3D, ELASTIC);
  m_targWSDescr.setLorentsCorr(false);

  // generate the detectors table
  Mantid::API::Algorithm_sptr childAlg = createChildAlgorithm(
      "PreprocessDetectorsToMD", 0.,
      .5); // HACK. soft dependency on non-dependent package.
  childAlg->setProperty("InputWorkspace", wksp);
  childAlg->executeAsChildAlg();

  DataObjects::TableWorkspace_sptr table =
      childAlg->getProperty("OutputWorkspace");
  if (!table)
    throw(std::runtime_error(
        "Can not retrieve results of \"PreprocessDetectorsToMD\""));
  else
    m_targWSDescr.m_PreprDetTable = table;

  auto numSpectra = static_cast<int>(wksp->getNumberHistograms());
  PARALLEL_FOR_IF(Kernel::threadSafe(*wksp))
  for (int i = 0; i < numSpectra; ++i) {
    PARALLEL_START_INTERUPT_REGION

    // units conversion helper
    UnitsConversionHelper unitConverter;
    unitConverter.initialize(m_targWSDescr, "Momentum");

    // initialize the MD coordinates conversion class
    MDTransfQ3D qConverter;
    qConverter.initialize(m_targWSDescr);

    // get tof and y values
    const auto &xVals = wksp->points(i);
    const auto &yVals = wksp->y(i);

    // update which pixel is being converted
    std::vector<Mantid::coord_t> locCoord(DIMS, 0.);
    unitConverter.updateConversion(i);
    qConverter.calcYDepCoordinates(locCoord, i);

    // loop over the events
    double signal(1.);  // ignorable garbage
    double errorSq(1.); // ignorable garbage

    std::vector<std::pair<double, V3D>> qList;

    for (size_t j = 0; j < yVals.size(); ++j) {
      const double &yVal = yVals[j];
      if (yVal > 0) // TODO, is this condition right?
      {
        double val = unitConverter.convertUnits(xVals[j]);
        qConverter.calcMatrixCoord(val, locCoord, signal, errorSq);
        V3D qVec(locCoord[0], locCoord[1], locCoord[2]);
        if (hkl_integ)
          qVec = UBinv * qVec;

        if (std::isnan(qVec[0]) || std::isnan(qVec[1]) || std::isnan(qVec[2]))
          continue;
        // Account for counts in histograms by increasing the qList with the
        // same q-point
        qList.emplace_back(yVal, qVec);
      }
    }
    PARALLEL_CRITICAL(addHisto) { integrator.addEvents(qList, hkl_integ); }
    prog.report();
    PARALLEL_END_INTERUPT_REGION
  } // end of loop over spectra
  PARALLEL_CHECK_INTERUPT_REGION
}

/*
 * Define edges for each instrument by masking. For CORELLI, tubes 1 and 16, and
 *pixels 0 and 255.
 * Get Q in the lab frame for every peak, call it C
 * For every point on the edge, the trajectory in reciprocal space is a straight
 *line, going through O=V3D(0,0,0).
 * Calculate a point at a fixed momentum, say k=1. Q in the lab frame
 *E=V3D(-k*sin(tt)*cos(ph),-k*sin(tt)*sin(ph),k-k*cos(ph)).
 * Normalize E to 1: E=E*(1./E.norm())
 *
 * @param inst: instrument
 */
void IntegrateEllipsoidsTwoStep::calculateE1(
    const Geometry::DetectorInfo &detectorInfo) {
  for (size_t i = 0; i < detectorInfo.size(); ++i) {
    if (detectorInfo.isMonitor(i))
      continue; // skip monitor
    if (!detectorInfo.isMasked(i))
      continue; // edge is masked so don't check if not masked
    const auto &det = detectorInfo.detector(i);
    double tt1 = det.getTwoTheta(V3D(0, 0, 0), V3D(0, 0, 1)); // two theta
    double ph1 = det.getPhi();                                // phi
    V3D E1 = V3D(-std::sin(tt1) * std::cos(ph1), -std::sin(tt1) * std::sin(ph1),
                 1. - std::cos(tt1)); // end of trajectory
    E1 = E1 * (1. / E1.norm());       // normalize
    E1Vec.push_back(E1);
  }
}

void IntegrateEllipsoidsTwoStep::runMaskDetectors(
    Mantid::DataObjects::PeaksWorkspace_sptr peakWS, std::string property,
    std::string values) {
  IAlgorithm_sptr alg = createChildAlgorithm("MaskBTP");
  alg->setProperty<Workspace_sptr>("Workspace", peakWS);
  alg->setProperty(property, values);
  if (!alg->execute())
    throw std::runtime_error(
        "MaskDetectors Child Algorithm has not executed successfully");
}
} // namespace MDAlgorithms
} // namespace Mantid
