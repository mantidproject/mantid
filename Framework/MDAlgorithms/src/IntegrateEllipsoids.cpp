// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/IntegrateEllipsoids.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/Statistics.h"
#include "MantidMDAlgorithms/Integrate3DEvents.h"
#include "MantidMDAlgorithms/MDTransfFactory.h"
#include "MantidMDAlgorithms/MDTransfQ3D.h"
#include "MantidMDAlgorithms/UnitsConversionHelper.h"

#include <boost/math/special_functions/round.hpp>
#include <cmath>

using namespace Mantid::API;
using namespace Mantid::HistogramData;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace MDAlgorithms {

/// This only works for diffraction.
const std::string ELASTIC("Elastic");

/// Only convert to Q-vector.
const std::string Q3D("Q3D");

/// Q-vector is always three dimensional.
const std::size_t DIMS(3);

/**
 * @brief qListFromEventWS creates qlist from events
 * @param integrator : itegrator object on which qlists are accumulated
 * @param prog : progress object
 * @param wksp : input EventWorkspace
 * @param UBinv : inverse of UB matrix
 * @param hkl_integ ; boolean for integrating in HKL space
 */
void IntegrateEllipsoids::qListFromEventWS(Integrate3DEvents &integrator,
                                           Progress &prog,
                                           EventWorkspace_sptr &wksp,
                                           DblMatrix const &UBinv,
                                           bool hkl_integ) {
  // loop through the eventlists

  int numSpectra = static_cast<int>(wksp->getNumberHistograms());
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
void IntegrateEllipsoids::qListFromHistoWS(Integrate3DEvents &integrator,
                                           Progress &prog,
                                           Workspace2D_sptr &wksp,
                                           DblMatrix const &UBinv,
                                           bool hkl_integ) {

  // loop through the eventlists

  int numSpectra = static_cast<int>(wksp->getNumberHistograms());
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
        for (size_t dim = 0; dim < DIMS; ++dim) {
          buffer[dim] = locCoord[dim]; // TODO. Looks un-necessary to me. Can't
                                       // we just add localCoord directly to
                                       // qVec
        }
        V3D qVec(buffer[0], buffer[1], buffer[2]);
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

/** NOTE: This has been adapted from the SaveIsawQvector algorithm.
 */

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(IntegrateEllipsoids)

//---------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string IntegrateEllipsoids::name() const {
  return "IntegrateEllipsoids";
}

/// Algorithm's version for identification. @see Algorithm::version
int IntegrateEllipsoids::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string IntegrateEllipsoids::category() const {
  return "Crystal\\Integration";
}

//---------------------------------------------------------------------

//---------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void IntegrateEllipsoids::init() {
  auto ws_valid = boost::make_shared<CompositeValidator>();
  ws_valid->add<WorkspaceUnitValidator>("TOF");
  ws_valid->add<InstrumentValidator>();
  // the validator which checks if the workspace has axis

  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input, ws_valid),
                  "An input MatrixWorkspace with time-of-flight units along "
                  "X-axis and defined instrument with defined sample");

  declareProperty(make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "PeaksWorkspace", "", Direction::InOut),
                  "Workspace with Peaks to be integrated. NOTE: The peaks MUST "
                  "be indexed with integer HKL values.");

  boost::shared_ptr<BoundedValidator<double>> mustBePositive(
      new BoundedValidator<double>());
  mustBePositive->setLower(0.0);

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

  declareProperty(
      make_unique<WorkspaceProperty<PeaksWorkspace>>("OutputWorkspace", "",
                                                     Direction::Output),
      "The output PeaksWorkspace will be a copy of the input PeaksWorkspace "
      "with the peaks' integrated intensities.");

  declareProperty("CutoffIsigI", EMPTY_DBL(), mustBePositive,
                  "Cuttoff for I/sig(i) when finding mean of half-length of "
                  "major radius in first pass when SpecifySize is false."
                  "Default is no second pass.");

  declareProperty("NumSigmas", 3,
                  "Number of sigmas to add to mean of half-length of "
                  "major radius for second pass when SpecifySize is false.");

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

  declareProperty("UseOnePercentBackgroundCorrection", true,
                  "If this options is enabled, then the the top 1% of the "
                  "background will be removed"
                  "before the background subtraction.");

  declareProperty("SatelliteRegionRadius", .1, mustBePositive,
                  "Only events at most this distance from a peak will be "
                  "considered when integrating");

  declareProperty("SatellitePeakSize", .08, mustBePositive,
                  "Half-length of major axis for satellite peak ellipsoid");

  declareProperty("SatelliteBackgroundInnerSize", .08, mustBePositive,
                  "Half-length of major axis for inner ellipsoidal surface of "
                  "satellite background region");

  declareProperty("SatelliteBackgroundOuterSize", .09, mustBePositive,
                  "Half-length of major axis for outer ellipsoidal surface of "
                  "satellite background region");
}

//---------------------------------------------------------------------
/** Execute the algorithm.
 */
void IntegrateEllipsoids::exec() {
  // get the input workspace
  MatrixWorkspace_sptr wksp = getProperty("InputWorkspace");

  EventWorkspace_sptr eventWS =
      boost::dynamic_pointer_cast<EventWorkspace>(wksp);
  Workspace2D_sptr histoWS = boost::dynamic_pointer_cast<Workspace2D>(wksp);
  if (!eventWS && !histoWS) {
    throw std::runtime_error("IntegrateEllipsoids needs either a "
                             "EventWorkspace or Workspace2D as input.");
  }

  // error out if there are not events
  if (eventWS && eventWS->getNumberEvents() <= 0) {
    throw std::runtime_error(
        "IntegrateEllipsoids does not work for empty event lists");
  }

  PeaksWorkspace_sptr in_peak_ws = getProperty("PeaksWorkspace");
  if (!in_peak_ws) {
    throw std::runtime_error("Could not read the peaks workspace");
  }

  double radius_m = getProperty("RegionRadius");
  double radius_s = getProperty("SatelliteRegionRadius");
  int numSigmas = getProperty("NumSigmas");
  double cutoffIsigI = getProperty("CutoffIsigI");
  bool specify_size = getProperty("SpecifySize");
  double peak_radius = getProperty("PeakSize");
  double sate_peak_radius = getProperty("SatellitePeakSize");
  double back_inner_radius = getProperty("BackgroundInnerSize");
  double sate_back_inner_radius = getProperty("SatelliteBackgroundInnerSize");
  double back_outer_radius = getProperty("BackgroundOuterSize");
  double sate_back_outer_radius = getProperty("SatelliteBackgroundOuterSize");
  bool hkl_integ = getProperty("IntegrateInHKL");
  bool integrateEdge = getProperty("IntegrateIfOnEdge");
  bool adaptiveQBackground = getProperty("AdaptiveQBackground");
  double adaptiveQMultiplier = getProperty("AdaptiveQMultiplier");
  double adaptiveQBackgroundMultiplier = 0.0;
  bool useOnePercentBackgroundCorrection =
      getProperty("UseOnePercentBackgroundCorrection");
  if (adaptiveQBackground)
    adaptiveQBackgroundMultiplier = adaptiveQMultiplier;
  if (!integrateEdge) {
    // This only fails in the unit tests which say that MaskBTP is not
    // registered
    try {
      runMaskDetectors(in_peak_ws, "Tube", "edges");
      runMaskDetectors(in_peak_ws, "Pixel", "edges");
    } catch (...) {
      g_log.error("Can't execute MaskBTP algorithm for this instrument to set "
                  "edge for IntegrateIfOnEdge option");
    }
    calculateE1(in_peak_ws->detectorInfo()); // fill E1Vec for use in detectorQ
  }

  Mantid::DataObjects::PeaksWorkspace_sptr peak_ws =
      getProperty("OutputWorkspace");
  if (peak_ws != in_peak_ws)
    peak_ws = in_peak_ws->clone();

  // get UBinv and the list of
  // peak Q's for the integrator
  std::vector<Peak> &peaks = peak_ws->getPeaks();
  size_t n_peaks = peak_ws->getNumberPeaks();
  size_t indexed_count = 0;
  std::vector<V3D> peak_q_list;
  std::vector<std::pair<double, V3D>> qList;
  std::vector<V3D> hkl_vectors;
  std::vector<V3D> mnp_vectors;
  int ModDim = 0;
  for (size_t i = 0; i < n_peaks; i++) // Note: we skip un-indexed peaks
  {
    V3D hkl(peaks[i].getIntHKL());
    V3D mnp(peaks[i].getIntMNP());

    if (mnp[0] != 0 && ModDim == 0)
      ModDim = 1;
    if (mnp[1] != 0 && ModDim == 1)
      ModDim = 2;
    if (mnp[2] != 0 && ModDim == 2)
      ModDim = 3;

    if (Geometry::IndexingUtils::ValidIndex(
            hkl, 1.0)) // use tolerance == 1 to
                       // just check for (0,0,0,0,0,0)
    {
      peak_q_list.emplace_back(peaks[i].getQLabFrame());
      qList.emplace_back(1., V3D(peaks[i].getQLabFrame()));
      hkl_vectors.push_back(hkl);
      mnp_vectors.push_back(mnp);
      indexed_count++;
    }
  }

  if (indexed_count < 3)
    throw std::runtime_error(
        "At least three linearly independent indexed peaks are needed.");

  // Get UB using indexed peaks and
  // lab-Q vectors
  Matrix<double> UB(3, 3, false);
  Matrix<double> modUB(3, 3, false);
  Matrix<double> modHKL(3, 3, false);
  Geometry::IndexingUtils::Optimize_6dUB(UB, modUB, hkl_vectors, mnp_vectors,
                                         ModDim, peak_q_list);

  int maxOrder = 0;
  bool CT = false;
  if (peak_ws->sample().hasOrientedLattice()) {
    OrientedLattice lattice = peak_ws->mutableSample().getOrientedLattice();
    lattice.setUB(UB);
    lattice.setModUB(modUB);
    modHKL = lattice.getModHKL();
    maxOrder = lattice.getMaxOrder();
    CT = lattice.getCrossTerm();
  }

  Matrix<double> UBinv(UB);
  UBinv.Invert();
  UBinv *= (1.0 / (2.0 * M_PI));

  std::vector<double> PeakRadiusVector(n_peaks, peak_radius);
  std::vector<double> BackgroundInnerRadiusVector(n_peaks, back_inner_radius);
  std::vector<double> BackgroundOuterRadiusVector(n_peaks, back_outer_radius);
  if (specify_size) {
    if (back_outer_radius > radius_m)
      throw std::runtime_error(
          "BackgroundOuterSize must be less than or equal to the RegionRadius");

    if (back_inner_radius >= back_outer_radius)
      throw std::runtime_error(
          "BackgroundInnerSize must be less BackgroundOuterSize");

    if (peak_radius > back_inner_radius)
      throw std::runtime_error(
          "PeakSize must be less than or equal to the BackgroundInnerSize");
  }

  // make the integrator
  Integrate3DEvents integrator(qList, hkl_vectors, mnp_vectors, UBinv, modHKL,
                               radius_m, radius_s, maxOrder, CT,
                               useOnePercentBackgroundCorrection);

  // get the events and add
  // them to the inegrator
  // set up a descripter of where we are going
  this->initTargetWSDescr(wksp);

  // set up the progress bar
  const size_t numSpectra = wksp->getNumberHistograms();
  Progress prog(this, 0.5, 1.0, numSpectra);

  if (eventWS) {
    // process as EventWorkspace
    qListFromEventWS(integrator, prog, eventWS, UBinv, hkl_integ);
  } else {
    // process as Workspace2D
    qListFromHistoWS(integrator, prog, histoWS, UBinv, hkl_integ);
  }

  double inti;
  double sigi;
  std::vector<double> principalaxis1, principalaxis2, principalaxis3;
  std::vector<double> sateprincipalaxis1, sateprincipalaxis2,
      sateprincipalaxis3;
  V3D peak_q;
  for (size_t i = 0; i < n_peaks; i++) {
    V3D hkl(peaks[i].getIntHKL());
    V3D mnp(peaks[i].getIntMNP());

    if (Geometry::IndexingUtils::ValidIndex(hkl, 1.0) ||
        Geometry::IndexingUtils::ValidIndex(mnp, 1.0)) {
      peak_q = peaks[i].getQLabFrame();
      std::vector<double> axes_radii;
      // modulus of Q
      double lenQpeak = 0.0;
      if (adaptiveQMultiplier != 0.0) {
        lenQpeak = peak_q.norm();
      }

      double adaptiveRadius = adaptiveQMultiplier * lenQpeak + peak_radius;
      if (mnp != V3D(0, 0, 0))
        adaptiveRadius = adaptiveQMultiplier * lenQpeak + sate_peak_radius;

      if (adaptiveRadius <= 0.0) {
        g_log.error() << "Error: Radius for integration sphere of peak " << i
                      << " is negative =  " << adaptiveRadius << '\n';
        peaks[i].setIntensity(0.0);
        peaks[i].setSigmaIntensity(0.0);
        PeakRadiusVector[i] = 0.0;
        BackgroundInnerRadiusVector[i] = 0.0;
        BackgroundOuterRadiusVector[i] = 0.0;
        continue;
      }

      double adaptiveBack_inner_radius =
          adaptiveQBackgroundMultiplier * lenQpeak + sate_back_inner_radius;
      if (mnp == V3D(0, 0, 0))
        adaptiveBack_inner_radius =
            adaptiveQBackgroundMultiplier * lenQpeak + back_inner_radius;

      double adaptiveBack_outer_radius =
          adaptiveQBackgroundMultiplier * lenQpeak + sate_back_outer_radius;
      if (mnp == V3D(0, 0, 0))
        adaptiveBack_outer_radius =
            adaptiveQBackgroundMultiplier * lenQpeak + back_outer_radius;

      PeakRadiusVector[i] = adaptiveRadius;
      BackgroundInnerRadiusVector[i] = adaptiveBack_inner_radius;
      BackgroundOuterRadiusVector[i] = adaptiveBack_outer_radius;

      Mantid::Geometry::PeakShape_const_sptr shape =
          integrator.ellipseIntegrateModEvents(
              E1Vec, peak_q, hkl, mnp, specify_size, adaptiveRadius,
              adaptiveBack_inner_radius, adaptiveBack_outer_radius, axes_radii,
              inti, sigi);
      peaks[i].setIntensity(inti);
      peaks[i].setSigmaIntensity(sigi);
      peaks[i].setPeakShape(shape);
      if (axes_radii.size() == 3) {
        if (inti / sigi > cutoffIsigI || cutoffIsigI == EMPTY_DBL()) {
          if (mnp == V3D(0, 0, 0)) {
            principalaxis1.push_back(axes_radii[0]);
            principalaxis2.push_back(axes_radii[1]);
            principalaxis3.push_back(axes_radii[2]);
          } else {
            sateprincipalaxis1.push_back(axes_radii[0]);
            sateprincipalaxis2.push_back(axes_radii[1]);
            sateprincipalaxis3.push_back(axes_radii[2]);
          }
        }
      }
    } else {
      peaks[i].setIntensity(0.0);
      peaks[i].setSigmaIntensity(0.0);
    }
  }
  if (principalaxis1.size() > 1) {
    Statistics stats1 = getStatistics(principalaxis1);
    g_log.notice() << "principalaxis1: "
                   << " mean " << stats1.mean << " standard_deviation "
                   << stats1.standard_deviation << " minimum " << stats1.minimum
                   << " maximum " << stats1.maximum << " median "
                   << stats1.median << "\n";
    Statistics stats2 = getStatistics(principalaxis2);
    g_log.notice() << "principalaxis2: "
                   << " mean " << stats2.mean << " standard_deviation "
                   << stats2.standard_deviation << " minimum " << stats2.minimum
                   << " maximum " << stats2.maximum << " median "
                   << stats2.median << "\n";
    Statistics stats3 = getStatistics(principalaxis3);
    g_log.notice() << "principalaxis3: "
                   << " mean " << stats3.mean << " standard_deviation "
                   << stats3.standard_deviation << " minimum " << stats3.minimum
                   << " maximum " << stats3.maximum << " median "
                   << stats3.median << "\n";

    if (sateprincipalaxis1.size() > 1) {
      Statistics satestats1 = getStatistics(sateprincipalaxis1);
      g_log.notice() << "sateprincipalaxis1: "
                     << " mean " << satestats1.mean << " standard_deviation "
                     << satestats1.standard_deviation << " minimum "
                     << satestats1.minimum << " maximum " << satestats1.maximum
                     << " median " << satestats1.median << "\n";
      Statistics satestats2 = getStatistics(sateprincipalaxis2);
      g_log.notice() << "sateprincipalaxis2: "
                     << " mean " << satestats2.mean << " standard_deviation "
                     << satestats2.standard_deviation << " minimum "
                     << satestats2.minimum << " maximum " << satestats2.maximum
                     << " median " << satestats2.median << "\n";
      Statistics satestats3 = getStatistics(sateprincipalaxis3);
      g_log.notice() << "sateprincipalaxis3: "
                     << " mean " << satestats3.mean << " standard_deviation "
                     << satestats3.standard_deviation << " minimum "
                     << satestats3.minimum << " maximum " << satestats3.maximum
                     << " median " << satestats3.median << "\n";
    }

    size_t histogramNumber = 3;
    Workspace_sptr wsProfile = WorkspaceFactory::Instance().create(
        "Workspace2D", histogramNumber, principalaxis1.size(),
        principalaxis1.size());
    Workspace2D_sptr wsProfile2D =
        boost::dynamic_pointer_cast<Workspace2D>(wsProfile);
    AnalysisDataService::Instance().addOrReplace("EllipsoidAxes", wsProfile2D);

    // set output workspace
    Points points(principalaxis1.size(), LinearGenerator(0, 1));
    wsProfile2D->setHistogram(0, points, Counts(std::move(principalaxis1)));
    wsProfile2D->setHistogram(1, points, Counts(std::move(principalaxis2)));
    wsProfile2D->setHistogram(2, points, Counts(std::move(principalaxis3)));

    if (cutoffIsigI != EMPTY_DBL()) {
      principalaxis1.clear();
      principalaxis2.clear();
      principalaxis3.clear();
      sateprincipalaxis1.clear();
      sateprincipalaxis2.clear();
      sateprincipalaxis3.clear();
      specify_size = true;
      peak_radius = std::max(std::max(stats1.mean, stats2.mean), stats3.mean) +
                    numSigmas * std::max(std::max(stats1.standard_deviation,
                                                  stats2.standard_deviation),
                                         stats3.standard_deviation);
      back_inner_radius = peak_radius;
      back_outer_radius = peak_radius * 1.25992105; // A factor of 2 ^ (1/3)
      // will make the background
      // shell volume equal to the peak region volume.
      V3D peak_q;
      for (size_t i = 0; i < n_peaks; i++) {
        V3D hkl(peaks[i].getIntHKL());
        V3D mnp(peaks[i].getIntMNP());
        if (Geometry::IndexingUtils::ValidIndex(hkl, 1.0) ||
            Geometry::IndexingUtils::ValidIndex(mnp, 1.0)) {
          peak_q = peaks[i].getQLabFrame();
          std::vector<double> axes_radii;
          integrator.ellipseIntegrateModEvents(
              E1Vec, peak_q, hkl, mnp, specify_size, peak_radius,
              back_inner_radius, back_outer_radius, axes_radii, inti, sigi);
          peaks[i].setIntensity(inti);
          peaks[i].setSigmaIntensity(sigi);
          if (axes_radii.size() == 3) {
            if (mnp == V3D(0, 0, 0)) {
              principalaxis1.push_back(axes_radii[0]);
              principalaxis2.push_back(axes_radii[1]);
              principalaxis3.push_back(axes_radii[2]);
            } else {
              sateprincipalaxis1.push_back(axes_radii[0]);
              sateprincipalaxis2.push_back(axes_radii[1]);
              sateprincipalaxis3.push_back(axes_radii[2]);
            }
          }
        } else {
          peaks[i].setIntensity(0.0);
          peaks[i].setSigmaIntensity(0.0);
        }
      }
      if (principalaxis1.size() > 1) {
        size_t histogramNumber = 3;
        Workspace_sptr wsProfile2 = WorkspaceFactory::Instance().create(
            "Workspace2D", histogramNumber, principalaxis1.size(),
            principalaxis1.size());
        Workspace2D_sptr wsProfile2D2 =
            boost::dynamic_pointer_cast<Workspace2D>(wsProfile2);
        AnalysisDataService::Instance().addOrReplace("EllipsoidAxes_2ndPass",
                                                     wsProfile2D2);

        // set output workspace
        Points points(principalaxis1.size(), LinearGenerator(0, 1));
        wsProfile2D->setHistogram(0, points, Counts(std::move(principalaxis1)));
        wsProfile2D->setHistogram(1, points, Counts(std::move(principalaxis2)));
        wsProfile2D->setHistogram(2, points, Counts(std::move(principalaxis3)));
      }
    }
  }

  // This flag is used by the PeaksWorkspace to evaluate whether it has been
  // integrated.
  peak_ws->mutableRun().addProperty("PeaksIntegrated", 1, true);
  // These flags are specific to the algorithm.
  peak_ws->mutableRun().addProperty("PeakRadius", PeakRadiusVector, true);
  peak_ws->mutableRun().addProperty("BackgroundInnerRadius",
                                    BackgroundInnerRadiusVector, true);
  peak_ws->mutableRun().addProperty("BackgroundOuterRadius",
                                    BackgroundOuterRadiusVector, true);

  setProperty("OutputWorkspace", peak_ws);
}

/**
 * @brief IntegrateEllipsoids::initTargetWSDescr Initialize the
 * output information for the MD conversion framework.
 *
 * @param wksp The workspace to get information from.
 */
void IntegrateEllipsoids::initTargetWSDescr(MatrixWorkspace_sptr &wksp) {
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

void IntegrateEllipsoids::calculateE1(
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

void IntegrateEllipsoids::runMaskDetectors(
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
