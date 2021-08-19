// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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
#include "MantidMDAlgorithms/IntegrateQLabEvents.h"
#include "MantidMDAlgorithms/MDTransfFactory.h"
#include "MantidMDAlgorithms/MDTransfQ3D.h"
#include "MantidMDAlgorithms/UnitsConversionHelper.h"

#include <cmath>

using namespace Mantid::API;
using namespace Mantid::HistogramData;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(IntegrateEllipsoids)

/// This only works for diffraction.
const std::string ELASTIC("Elastic");

/// Only convert to Q-vector.
const std::string Q3D("Q3D");

/// Q-vector is always three dimensional.
const std::size_t DIMS(3);

void IntegrateEllipsoids::qListFromEventWS(IntegrateQLabEvents &integrator, Progress &prog, EventWorkspace_sptr &wksp) {
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
    PARALLEL_END_INTERUPT_REGION
  } // end of loop over spectra
  PARALLEL_CHECK_INTERUPT_REGION
  integrator.populateCellsWithPeaks();
}

void IntegrateEllipsoids::qListFromHistoWS(IntegrateQLabEvents &integrator, Progress &prog, Workspace2D_sptr &wksp) {
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
    PARALLEL_END_INTERUPT_REGION
  } // end of loop over spectra
  PARALLEL_CHECK_INTERUPT_REGION
  integrator.populateCellsWithPeaks();
}

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
  declareProperty(
      "SatelliteBackgroundInnerSize", EMPTY_DBL(), mustBePositive,
      "Half-length of major axis for the inner ellipsoidal surface of background region of the satellite peak");
  declareProperty(
      "SatelliteBackgroundOuterSize", EMPTY_DBL(), mustBePositive,
      "Half-length of major axis for the outer ellipsoidal surface of background region of the satellite peak");
}

/**
 * @brief validate input properties
 *
 * @return std::map<std::string, std::string>
 */
std::map<std::string, std::string> IntegrateEllipsoids::validateInputs() {
  std::map<std::string, std::string> issues;

  // case 1: specified peak and background must be realisitc
  double radius_m = getProperty("RegionRadius");
  bool specify_size = getProperty("SpecifySize");
  double peak_radius = getProperty("PeakSize");
  double back_inner_radius = getProperty("BackgroundInnerSize");
  double back_outer_radius = getProperty("BackgroundOuterSize");
  if (specify_size) {
    if (back_outer_radius > radius_m) {
      issues["SpecifySize"] = "BackgroundOuterSize must be less than or equal to the RegionRadius";
    }
    if (back_inner_radius >= back_outer_radius) {
      issues["SpecifySize"] = "BackgroundInnerSize must be less than BackgroundOuterSize";
    }
    if (peak_radius > back_inner_radius) {
      issues["SpecifySize"] = "PeakSize must be less than or equal to the BackgroundInnerSize";
    }
  }

  // case 2: specified satellite peak and background must be realisitc
  double satellite_radius = (getPointerToProperty("SatelliteRegionRadius")->isDefault())
                                ? getProperty("RegionRadius")
                                : getProperty("SatelliteRegionRadius");
  double satellite_peak_radius = (getPointerToProperty("SatellitePeakSize")->isDefault())
                                     ? getProperty("PeakSize")
                                     : getProperty("SatellitePeakSize");
  double satellite_back_inner_radius = (getPointerToProperty("SatelliteBackgroundInnerSize")->isDefault())
                                           ? getProperty("BackgroundInnerSize")
                                           : getProperty("SatelliteBackgroundInnerSize");
  double satellite_back_outer_radius = (getPointerToProperty("SatelliteBackgroundOuterSize")->isDefault())
                                           ? getProperty("BackgroundOuterSize")
                                           : getProperty("SatelliteBackgroundOuterSize");
  if (specify_size) {
    if (satellite_back_outer_radius > satellite_radius) {
      issues["SpecifySize"] = "SatelliteBackgroundOuterSize must be less than or equal to the SatelliteRegionRadius";
    }
    if (satellite_back_inner_radius > satellite_back_outer_radius) {
      issues["SpecifySize"] = "SatelliteBackgroundInnerSize must be less than SatelliteBackgroundOuterSize";
    }
    if (satellite_peak_radius > satellite_back_inner_radius) {
      issues["SpecifySize"] = "SatellitePeakSize must be less than or equal to the SatelliteBackgroundInnerSize";
    }
  }

  // case 3: anything else?

  return issues;
}

void IntegrateEllipsoids::exec() {
  // get the input workspace
  MatrixWorkspace_sptr wksp = getProperty("InputWorkspace");

  EventWorkspace_sptr eventWS = std::dynamic_pointer_cast<EventWorkspace>(wksp);
  Workspace2D_sptr histoWS = std::dynamic_pointer_cast<Workspace2D>(wksp);
  if (!eventWS && !histoWS) {
    throw std::runtime_error("IntegrateEllipsoids needs either a "
                             "EventWorkspace or Workspace2D as input.");
  }

  // error out if there are not events
  if (eventWS && eventWS->getNumberEvents() <= 0) {
    throw std::runtime_error("IntegrateEllipsoids does not work for empty event lists");
  }

  PeaksWorkspace_sptr in_peak_ws = getProperty("PeaksWorkspace");
  if (!in_peak_ws) {
    throw std::runtime_error("Could not read the peaks workspace");
  }

  double radius_m = getProperty("RegionRadius");
  int numSigmas = getProperty("NumSigmas");
  double cutoffIsigI = getProperty("CutoffIsigI");
  bool specify_size = getProperty("SpecifySize");
  double peak_radius = getProperty("PeakSize");
  double back_inner_radius = getProperty("BackgroundInnerSize");
  double back_outer_radius = getProperty("BackgroundOuterSize");
  bool integrateEdge = getProperty("IntegrateIfOnEdge");
  bool adaptiveQBackground = getProperty("AdaptiveQBackground");
  double adaptiveQMultiplier = getProperty("AdaptiveQMultiplier");
  double adaptiveQBackgroundMultiplier = 0.0;
  bool useOnePercentBackgroundCorrection = getProperty("UseOnePercentBackgroundCorrection");
  // satellite related properties
  // NOTE: fallback to Brag Peak properties if satellite peak related properties are not specified
  double satellite_radius = (getPointerToProperty("SatelliteRegionRadius")->isDefault())
                                ? getProperty("RegionRadius")
                                : getProperty("SatelliteRegionRadius");
  double satellite_peak_radius = (getPointerToProperty("SatellitePeakSize")->isDefault())
                                     ? getProperty("PeakSize")
                                     : getProperty("SatellitePeakSize");
  double satellite_back_inner_radius = (getPointerToProperty("SatelliteBackgroundInnerSize")->isDefault())
                                           ? getProperty("BackgroundInnerSize")
                                           : getProperty("SatelliteBackgroundInnerSize");
  double satellite_back_outer_radius = (getPointerToProperty("SatelliteBackgroundOuterSize")->isDefault())
                                           ? getProperty("BackgroundOuterSize")
                                           : getProperty("SatelliteBackgroundOuterSize");

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

  Mantid::DataObjects::PeaksWorkspace_sptr peak_ws = getProperty("OutputWorkspace");
  if (peak_ws != in_peak_ws)
    peak_ws = in_peak_ws->clone();

  // get the list of peak Q's for the integrator
  std::vector<Peak> &peaks = peak_ws->getPeaks();
  size_t n_peaks = peak_ws->getNumberPeaks();
  SlimEvents qList;
  // Note: we skip un-indexed peaks
  for (size_t i = 0; i < n_peaks; i++) {
    // check if peak is satellite peak
    const bool isSatellitePeak = (peaks[i].getIntMNP().norm2() > 0);
    const V3D peak_q = peaks[i].getQLabFrame();
    const bool isOrigin = isSatellitePeak ? IntegrateQLabEvents::isOrigin(peak_q, satellite_radius)
                                          : IntegrateQLabEvents::isOrigin(peak_q, radius_m);
    if (isOrigin) {
      continue; // skip this peak
    }
    // add peak Q to list
    V3D hkl(peaks[i].getIntHKL());
    // use tolerance == 1 to just check for (0,0,0,0,0,0)
    if (Geometry::IndexingUtils::ValidIndex(hkl, 1.0)) {
      qList.emplace_back(std::pair<double, double>(1., 1.), peak_q);
    }
  }

  // Peak vectors
  std::vector<double> PeakRadiusVector(n_peaks, peak_radius);
  std::vector<double> BackgroundInnerRadiusVector(n_peaks, back_inner_radius);
  std::vector<double> BackgroundOuterRadiusVector(n_peaks, back_outer_radius);
  // Satellite peak vectors
  std::vector<double> SatellitePeakRadiusVector(n_peaks, satellite_peak_radius);
  std::vector<double> SatelliteBackgroundInnerRadiusVector(n_peaks, satellite_back_inner_radius);
  std::vector<double> SatelliteBackgroundOuterRadiusVector(n_peaks, satellite_back_outer_radius);

  // make the integrator
  IntegrateQLabEvents integrator(qList, radius_m, useOnePercentBackgroundCorrection);
  IntegrateQLabEvents integrator_satellite(qList, satellite_radius, useOnePercentBackgroundCorrection);

  // get the events and add
  // them to the inegrator
  // set up a descripter of where we are going
  this->initTargetWSDescr(wksp);

  // set up the progress bar
  const size_t numSpectra = wksp->getNumberHistograms();
  Progress prog(this, 0.5, 1.0, numSpectra);

  if (eventWS) {
    // process as EventWorkspace
    qListFromEventWS(integrator, prog, eventWS);
    qListFromEventWS(integrator_satellite, prog, eventWS);
  } else {
    // process as Workspace2D
    qListFromHistoWS(integrator, prog, histoWS);
    qListFromHistoWS(integrator_satellite, prog, histoWS);
  }

  double inti;
  double sigi;
  std::vector<double> principalaxis1, principalaxis2, principalaxis3;
  for (size_t i = 0; i < n_peaks; i++) {
    // check if peak is satellite peak
    const bool isSatellitePeak = (peaks[i].getIntMNP().norm2() > 0);
    // grab QLabFrame
    const V3D peak_q = peaks[i].getQLabFrame();
    // check if peak is origin (skip if true)
    const bool isOrigin = isSatellitePeak ? IntegrateQLabEvents::isOrigin(peak_q, satellite_radius)
                                          : IntegrateQLabEvents::isOrigin(peak_q, radius_m);
    if (isOrigin) {
      continue;
    }
    // modulus of Q
    const double lenQpeak = (adaptiveQMultiplier != 0.0) ? peak_q.norm() : 0.0;
    // compuate adaptive radius
    double adaptiveRadius = isSatellitePeak ? adaptiveQMultiplier * lenQpeak + satellite_peak_radius
                                            : adaptiveQMultiplier * lenQpeak + peak_radius;
    // - error checking for adaptive radius
    if (adaptiveRadius < 0.0) {
      std::ostringstream errmsg;
      errmsg << "Error: Radius for integration sphere of peak " << i << " is negative =  " << adaptiveRadius << '\n';
      g_log.error() << errmsg.str();
      // zero the peak
      peaks[i].setIntensity(0.0);
      peaks[i].setSigmaIntensity(0.0);
      PeakRadiusVector[i] = 0.0;
      BackgroundInnerRadiusVector[i] = 0.0;
      BackgroundOuterRadiusVector[i] = 0.0;
      SatellitePeakRadiusVector[i] = 0.0;
      SatelliteBackgroundInnerRadiusVector[i] = 0.0;
      SatelliteBackgroundOuterRadiusVector[i] = 0.0;
      continue;
    }
    // compute adaptive background radius
    double adaptiveBack_inner_radius = isSatellitePeak
                                           ? adaptiveQBackgroundMultiplier * lenQpeak + satellite_back_inner_radius
                                           : adaptiveQBackgroundMultiplier * lenQpeak + back_inner_radius;
    double adaptiveBack_outer_radius = isSatellitePeak
                                           ? adaptiveQBackgroundMultiplier * lenQpeak + satellite_back_outer_radius
                                           : adaptiveQBackgroundMultiplier * lenQpeak + back_outer_radius;
    // update records in containers
    if (isSatellitePeak) {
      SatellitePeakRadiusVector[i] = adaptiveRadius;
      SatelliteBackgroundInnerRadiusVector[i] = adaptiveBack_inner_radius;
      SatelliteBackgroundOuterRadiusVector[i] = adaptiveBack_outer_radius;
    } else {
      PeakRadiusVector[i] = adaptiveRadius;
      BackgroundInnerRadiusVector[i] = adaptiveBack_inner_radius;
      BackgroundOuterRadiusVector[i] = adaptiveBack_outer_radius;
    }

    // integrate the peak to get intensity and error
    std::vector<double> axes_radii;
    Mantid::Geometry::PeakShape_const_sptr shape;
    if (isSatellitePeak) {
      shape = integrator_satellite.ellipseIntegrateEvents(E1Vec, peak_q, specify_size, adaptiveRadius,
                                                          adaptiveBack_inner_radius, adaptiveBack_outer_radius,
                                                          axes_radii, inti, sigi);
    } else {
      shape = integrator.ellipseIntegrateEvents(E1Vec, peak_q, specify_size, adaptiveRadius, adaptiveBack_inner_radius,
                                                adaptiveBack_outer_radius, axes_radii, inti, sigi);
    }

    peaks[i].setIntensity(inti);
    peaks[i].setSigmaIntensity(sigi);
    peaks[i].setPeakShape(shape);
    if (axes_radii.size() == 3) {
      if (inti / sigi > cutoffIsigI || cutoffIsigI == EMPTY_DBL()) {
        principalaxis1.emplace_back(axes_radii[0]);
        principalaxis2.emplace_back(axes_radii[1]);
        principalaxis3.emplace_back(axes_radii[2]);
      }
    }
  }
  if (principalaxis1.size() > 1) {
    Statistics stats1 = getStatistics(principalaxis1);
    g_log.notice() << "principalaxis1: "
                   << " mean " << stats1.mean << " standard_deviation " << stats1.standard_deviation << " minimum "
                   << stats1.minimum << " maximum " << stats1.maximum << " median " << stats1.median << "\n";
    Statistics stats2 = getStatistics(principalaxis2);
    g_log.notice() << "principalaxis2: "
                   << " mean " << stats2.mean << " standard_deviation " << stats2.standard_deviation << " minimum "
                   << stats2.minimum << " maximum " << stats2.maximum << " median " << stats2.median << "\n";
    Statistics stats3 = getStatistics(principalaxis3);
    g_log.notice() << "principalaxis3: "
                   << " mean " << stats3.mean << " standard_deviation " << stats3.standard_deviation << " minimum "
                   << stats3.minimum << " maximum " << stats3.maximum << " median " << stats3.median << "\n";

    constexpr size_t histogramNumber = 3;
    Workspace_sptr wsProfile = WorkspaceFactory::Instance().create("Workspace2D", histogramNumber,
                                                                   principalaxis1.size(), principalaxis1.size());
    Workspace2D_sptr wsProfile2D = std::dynamic_pointer_cast<Workspace2D>(wsProfile);
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
      specify_size = true;
      double meanMax = std::max(std::max(stats1.mean, stats2.mean), stats3.mean);
      double stdMax =
          std::max(std::max(stats1.standard_deviation, stats2.standard_deviation), stats3.standard_deviation);
      peak_radius = meanMax + numSigmas * stdMax;
      back_inner_radius = peak_radius;
      back_outer_radius = peak_radius * 1.25992105; // A factor of 2 ^ (1/3)
      // will make the background shell volume equal to the peak region volume.
      for (size_t i = 0; i < n_peaks; i++) {
        // check if peak is satellite peak
        const bool isSatellitePeak = (peaks[i].getIntMNP().norm2() > 0);
        //
        const V3D peak_q = peaks[i].getQLabFrame();
        std::vector<double> axes_radii;

        if (isSatellitePeak) {
          integrator_satellite.ellipseIntegrateEvents(E1Vec, peak_q, specify_size, peak_radius, back_inner_radius,
                                                      back_outer_radius, axes_radii, inti, sigi);
        } else {
          integrator.ellipseIntegrateEvents(E1Vec, peak_q, specify_size, peak_radius, back_inner_radius,
                                            back_outer_radius, axes_radii, inti, sigi);
        }

        peaks[i].setIntensity(inti);
        peaks[i].setSigmaIntensity(sigi);
        if (axes_radii.size() == 3) {
          principalaxis1.emplace_back(axes_radii[0]);
          principalaxis2.emplace_back(axes_radii[1]);
          principalaxis3.emplace_back(axes_radii[2]);
        }
      }
      if (principalaxis1.size() > 1) {
        Workspace_sptr wsProfile2 = WorkspaceFactory::Instance().create("Workspace2D", histogramNumber,
                                                                        principalaxis1.size(), principalaxis1.size());
        Workspace2D_sptr wsProfile2D2 = std::dynamic_pointer_cast<Workspace2D>(wsProfile2);
        AnalysisDataService::Instance().addOrReplace("EllipsoidAxes_2ndPass", wsProfile2D2);

        Points profilePoints(principalaxis1.size(), LinearGenerator(0, 1));
        wsProfile2D->setHistogram(0, profilePoints, Counts(std::move(principalaxis1)));
        wsProfile2D->setHistogram(1, profilePoints, Counts(std::move(principalaxis2)));
        wsProfile2D->setHistogram(2, profilePoints, Counts(std::move(principalaxis3)));
      }
    }
  }

  // This flag is used by the PeaksWorkspace to evaluate whether it has been
  // integrated.
  peak_ws->mutableRun().addProperty("PeaksIntegrated", 1, true);
  // These flags are specific to the algorithm.
  peak_ws->mutableRun().addProperty("PeakRadius", PeakRadiusVector, true);
  peak_ws->mutableRun().addProperty("BackgroundInnerRadius", BackgroundInnerRadiusVector, true);
  peak_ws->mutableRun().addProperty("BackgroundOuterRadius", BackgroundOuterRadiusVector, true);
  // These falgs are related to the satellite peaks and specific to the algorithm.
  peak_ws->mutableRun().addProperty("SatellitePeakRadius", SatellitePeakRadiusVector, true);
  peak_ws->mutableRun().addProperty("SatelliteBackgroundInnerRadius", SatelliteBackgroundInnerRadiusVector, true);
  peak_ws->mutableRun().addProperty("SatelliteBackgroundOuterRadius", SatelliteBackgroundOuterRadiusVector, true);

  setProperty("OutputWorkspace", peak_ws);
}

void IntegrateEllipsoids::initTargetWSDescr(MatrixWorkspace_sptr &wksp) {
  m_targWSDescr.setMinMax(std::vector<double>(3, -2000.), std::vector<double>(3, 2000.));
  m_targWSDescr.buildFromMatrixWS(wksp, Q3D, ELASTIC);
  m_targWSDescr.setLorentsCorr(false);

  // generate the detectors table
  Mantid::API::Algorithm_sptr childAlg = createChildAlgorithm("PreprocessDetectorsToMD", 0.,
                                                              .5); // HACK. soft dependency on non-dependent package.
  childAlg->setProperty("InputWorkspace", wksp);
  childAlg->executeAsChildAlg();

  DataObjects::TableWorkspace_sptr table = childAlg->getProperty("OutputWorkspace");
  if (!table)
    throw(std::runtime_error("Can not retrieve results of \"PreprocessDetectorsToMD\""));
  else
    m_targWSDescr.m_PreprDetTable = table;
}

void IntegrateEllipsoids::calculateE1(const Geometry::DetectorInfo &detectorInfo) {
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
    E1Vec.emplace_back(E1);
  }
}

void IntegrateEllipsoids::runMaskDetectors(const Mantid::DataObjects::PeaksWorkspace_sptr &peakWS,
                                           const std::string &property, const std::string &values) {
  auto alg = createChildAlgorithm("MaskBTP");
  alg->setProperty<Workspace_sptr>("Workspace", peakWS);
  alg->setProperty(property, values);
  if (!alg->execute())
    throw std::runtime_error("MaskDetectors Child Algorithm has not executed successfully");
}
} // namespace MDAlgorithms
} // namespace Mantid
