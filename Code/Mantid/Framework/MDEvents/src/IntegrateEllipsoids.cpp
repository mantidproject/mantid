#include <iostream>
#include <fstream>
#include <boost/math/special_functions/round.hpp>
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/V3D.h"
#include "MantidMDEvents/MDTransfFactory.h"
#include "MantidMDEvents/UnitsConversionHelper.h"
#include "MantidMDEvents/Integrate3DEvents.h"
#include "MantidMDEvents/IntegrateEllipsoids.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Statistics.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

/// This only works for diffraction.
const std::string ELASTIC("Elastic");

/// Only convert to Q-vector.
const std::string Q3D("Q3D");

/// Q-vector is always three dimensional.
const std::size_t DIMS(3);

namespace {

using namespace Mantid::MDEvents;

/**
 * @brief qListFromEventWS creates qlist from events
 * @param integrator : itegrator object on which qlists are accumulated
 * @param prog : progress object
 * @param wksp : input EventWorkspace
 * @param unitConverter : Unit converter
 * @param qConverter : Q converter
 */
void qListFromEventWS(Integrate3DEvents &integrator, Progress &prog,
                      EventWorkspace_sptr &wksp,
                      UnitsConversionHelper &unitConverter,
                      MDTransf_sptr &qConverter) {
  // loop through the eventlists
  std::vector<double> buffer(DIMS);

  size_t numSpectra = wksp->getNumberHistograms();
  for (std::size_t i = 0; i < numSpectra; ++i) {
    // get a reference to the event list
    EventList &events = wksp->getEventList(i);

    events.switchTo(WEIGHTED_NOTIME);

    // check to see if the event list is empty
    if (events.empty()) {
      prog.report();
      continue; // nothing to do
    }

    // update which pixel is being converted
    std::vector<Mantid::coord_t> locCoord(DIMS, 0.);
    unitConverter.updateConversion(i);
    qConverter->calcYDepCoordinates(locCoord, i);

    // loop over the events
    double signal(1.);  // ignorable garbage
    double errorSq(1.); // ignorable garbage
    const std::vector<WeightedEventNoTime> &raw_events =
        events.getWeightedEventsNoTime();
    std::vector<std::pair<double, V3D> > qList;
    for (auto event = raw_events.begin(); event != raw_events.end(); ++event) {
      double val = unitConverter.convertUnits(event->tof());
      qConverter->calcMatrixCoord(val, locCoord, signal, errorSq);
      for (size_t dim = 0; dim < DIMS; ++dim) {
        buffer[dim] = locCoord[dim];
      }
      V3D qVec(buffer[0], buffer[1], buffer[2]);
      qList.push_back(std::make_pair(event->m_weight, qVec));
    } // end of loop over events in list

    integrator.addEvents(qList);

    prog.report();
  } // end of loop over spectra
}

/**
 * @brief qListFromHistoWS creates qlist from input workspaces of type Workspace2D
 * @param integrator : itegrator object on which qlists are accumulated
 * @param prog : progress object
 * @param wksp : input Workspace2D
 * @param unitConverter : Unit converter
 * @param qConverter : Q converter
 */
void qListFromHistoWS(Integrate3DEvents &integrator, Progress &prog,
                      Workspace2D_sptr &wksp,
                      UnitsConversionHelper &unitConverter,
                      MDTransf_sptr &qConverter) {

  // loop through the eventlists
  std::vector<double> buffer(DIMS);

  size_t numSpectra = wksp->getNumberHistograms();
  const bool histogramForm = wksp->isHistogramData();
  for (std::size_t i = 0; i < numSpectra; ++i) {
    // get tof and counts
    const Mantid::MantidVec &xVals = wksp->readX(i);
    const Mantid::MantidVec &yVals = wksp->readY(i);

    // update which pixel is being converted
    std::vector<Mantid::coord_t> locCoord(DIMS, 0.);
    unitConverter.updateConversion(i);
    qConverter->calcYDepCoordinates(locCoord, i);

    // loop over the events
    double signal(1.);  // ignorable garbage
    double errorSq(1.); // ignorable garbage

    std::vector<std::pair<double, V3D> > qList;

    // TODO. we should be able to do this in an OMP loop.
    for (size_t j = 0; j < yVals.size(); ++j) {
      const double &yVal = yVals[j];
      if (yVal > 0) // TODO, is this condition right?
      {
        // Tof from point data
        double tof = xVals[j];
        if (histogramForm) {
          // Tof is the centre point
          tof = (tof + xVals[j + 1]) / 2;
        }

        double val = unitConverter.convertUnits(tof);
        qConverter->calcMatrixCoord(val, locCoord, signal, errorSq);
        for (size_t dim = 0; dim < DIMS; ++dim) {
          buffer[dim] = locCoord[dim]; // TODO. Looks un-necessary to me. Can't
                                       // we just add localCoord directly to
                                       // qVec
        }
        V3D qVec(buffer[0], buffer[1], buffer[2]);
        int yValCounts = int(yVal); // we deliberately truncate.
        // Account for counts in histograms by increasing the qList with the
        // same q-point
          qList.push_back(std::make_pair(yValCounts,qVec)); // Not ideal to control the size dynamically?
      }
      integrator.addEvents(qList); // We would have to put a lock around this.
      prog.report();
    }

    integrator.addEvents(qList);

    prog.report();
  }
}
}

namespace Mantid {
namespace MDEvents {
/** NOTE: This has been adapted from the SaveIsawQvector algorithm.
 */

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(IntegrateEllipsoids)

//----------------------------------------------------------------------
/** Constructor
 */
IntegrateEllipsoids::IntegrateEllipsoids() {}

//---------------------------------------------------------------------
/** Destructor
 */
IntegrateEllipsoids::~IntegrateEllipsoids() {}

//---------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string IntegrateEllipsoids::name() const {
  return "IntegrateEllipsoids";
}

/// Algorithm's version for identification. @see Algorithm::version
int IntegrateEllipsoids::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string IntegrateEllipsoids::category() const { return "Crystal"; }

//---------------------------------------------------------------------

//---------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void IntegrateEllipsoids::init() {
  auto ws_valid = boost::make_shared<CompositeValidator>();

  ws_valid->add<InstrumentValidator>();
  // the validator which checks if the workspace has axis

  declareProperty(new WorkspaceProperty<MatrixWorkspace>(
                      "InputWorkspace", "", Direction::Input, ws_valid),
                  "An input MatrixWorkspace with time-of-flight units along "
                  "X-axis and defined instrument with defined sample");

  declareProperty(new WorkspaceProperty<PeaksWorkspace>("PeaksWorkspace", "",
                                                        Direction::InOut),
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
      new WorkspaceProperty<PeaksWorkspace>("OutputWorkspace", "",
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

  Mantid::DataObjects::PeaksWorkspace_sptr peak_ws =
      getProperty("OutputWorkspace");
  if (peak_ws != in_peak_ws) {
    peak_ws = in_peak_ws->clone();
  }
  double radius = getProperty("RegionRadius");
  int numSigmas = getProperty("NumSigmas");
  double cutoffIsigI = getProperty("CutoffIsigI");
  bool specify_size = getProperty("SpecifySize");
  double peak_radius = getProperty("PeakSize");
  double back_inner_radius = getProperty("BackgroundInnerSize");
  double back_outer_radius = getProperty("BackgroundOuterSize");
  // get UBinv and the list of
  // peak Q's for the integrator
  std::vector<Peak> &peaks = peak_ws->getPeaks();
  size_t n_peaks = peak_ws->getNumberPeaks();
  size_t indexed_count = 0;
  std::vector<V3D> peak_q_list;
  std::vector<std::pair<double, V3D> > qList;
  std::vector<V3D> hkl_vectors;
  for (size_t i = 0; i < n_peaks; i++) // Note: we skip un-indexed peaks
  {
    V3D hkl(peaks[i].getH(), peaks[i].getK(), peaks[i].getL());
    if (Geometry::IndexingUtils::ValidIndex(hkl, 1.0)) // use tolerance == 1 to
                                                       // just check for (0,0,0)
    {
      peak_q_list.push_back(V3D(peaks[i].getQLabFrame()));
      qList.push_back(std::make_pair(1., V3D(peaks[i].getQLabFrame())));
      V3D miller_ind((double)boost::math::iround<double>(hkl[0]),
                     (double)boost::math::iround<double>(hkl[1]),
                     (double)boost::math::iround<double>(hkl[2]));
      hkl_vectors.push_back(V3D(miller_ind));
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

  std::vector<double> PeakRadiusVector(n_peaks, peak_radius);
  std::vector<double> BackgroundInnerRadiusVector(n_peaks, back_inner_radius);
  std::vector<double> BackgroundOuterRadiusVector(n_peaks, back_outer_radius);
  if (specify_size) {
    if (back_outer_radius > radius)
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
  Integrate3DEvents integrator(qList, UBinv, radius);

  // get the events and add
  // them to the inegrator
  // set up a descripter of where we are going
  this->initTargetWSDescr(wksp);

  // units conversion helper
  UnitsConversionHelper unitConv;
  unitConv.initialize(m_targWSDescr, "Momentum");

  // initialize the MD coordinates conversion class
  MDTransf_sptr qConverter =
      MDTransfFactory::Instance().create(m_targWSDescr.AlgID);
  qConverter->initialize(m_targWSDescr);

  // set up the progress bar
  const size_t numSpectra = wksp->getNumberHistograms();
  Progress prog(this, 0.5, 1.0, numSpectra);

  // loop through the eventlists
  std::vector<double> buffer(DIMS);

  if (eventWS) {
    // process as EventWorkspace
    qListFromEventWS(integrator, prog, eventWS, unitConv, qConverter);
  } else {
    // process as Workspace2D
    qListFromHistoWS(integrator, prog, histoWS, unitConv, qConverter);
  }

  double inti;
  double sigi;
  std::vector<double> principalaxis1,principalaxis2,principalaxis3;
  std::pair <double, V3D> peak_q;
  for (size_t i = 0; i < n_peaks; i++) {
    V3D hkl(peaks[i].getH(), peaks[i].getK(), peaks[i].getL());
    if (Geometry::IndexingUtils::ValidIndex(hkl, 1.0)) {
      peak_q.first = 1.;
      peak_q.second = peaks[i].getQLabFrame();
      std::vector<double> axes_radii;
      Mantid::Geometry::PeakShape_const_sptr shape =
          integrator.ellipseIntegrateEvents(
              peak_q, specify_size, peak_radius, back_inner_radius,
              back_outer_radius, axes_radii, inti, sigi);
      peaks[i].setIntensity(inti);
      peaks[i].setSigmaIntensity(sigi);
      peaks[i].setPeakShape(shape);
      if (axes_radii.size() == 3) {
        if (inti/sigi > cutoffIsigI || cutoffIsigI == EMPTY_DBL()){
          principalaxis1.push_back(axes_radii[0]);
          principalaxis2.push_back(axes_radii[1]);
          principalaxis3.push_back(axes_radii[2]);
        }
      }
    } else {
      peaks[i].setIntensity(0.0);
      peaks[i].setSigmaIntensity(0.0);
    }
  }
  if (principalaxis1.size() > 1 ){
    size_t histogramNumber = 3;
    Workspace_sptr wsProfile = WorkspaceFactory::Instance().create(
        "Workspace2D", histogramNumber, principalaxis1.size(), principalaxis1.size());
    Workspace2D_sptr wsProfile2D = boost::dynamic_pointer_cast<Workspace2D>(wsProfile);
    AnalysisDataService::Instance().addOrReplace("EllipsoidAxes", wsProfile2D);
    for (size_t j = 0; j < principalaxis1.size(); j++) {
      wsProfile2D->dataX(0)[j] = static_cast<double>(j);
      wsProfile2D->dataY(0)[j] = principalaxis1[j];
      wsProfile2D->dataE(0)[j] = std::sqrt(principalaxis1[j]);
      wsProfile2D->dataX(1)[j] = static_cast<double>(j);
      wsProfile2D->dataY(1)[j] = principalaxis2[j];
      wsProfile2D->dataE(1)[j] = std::sqrt(principalaxis2[j]);
      wsProfile2D->dataX(2)[j] = static_cast<double>(j);
      wsProfile2D->dataY(2)[j] = principalaxis3[j];
      wsProfile2D->dataE(2)[j] = std::sqrt(principalaxis3[j]);
    }
    Statistics stats1 = getStatistics(principalaxis1);
    g_log.notice() << "principalaxis1: "
        << " mean " << stats1.mean
        << " standard_deviation " << stats1.standard_deviation
        << " minimum " << stats1.minimum
        << " maximum " << stats1.maximum
        << " median " << stats1.median << "\n";
    Statistics stats2 = getStatistics(principalaxis2);
    g_log.notice() << "principalaxis2: "
        << " mean " << stats2.mean
        << " standard_deviation " << stats2.standard_deviation
        << " minimum " << stats2.minimum
        << " maximum " << stats2.maximum
        << " median " << stats2.median << "\n";
    Statistics stats3 = getStatistics(principalaxis3);
    g_log.notice() << "principalaxis3: "
        << " mean " << stats3.mean
        << " standard_deviation " << stats3.standard_deviation
        << " minimum " << stats3.minimum
        << " maximum " << stats3.maximum
        << " median " << stats3.median << "\n";
    if (cutoffIsigI != EMPTY_DBL()){
      principalaxis1.clear();
      principalaxis2.clear();
      principalaxis3.clear();
      specify_size=true;
      peak_radius = std::max(std::max(stats1.mean,stats2.mean),stats3.mean) + numSigmas *
        std::max(std::max(stats1.standard_deviation,stats2.standard_deviation),stats3.standard_deviation);
      back_inner_radius = peak_radius;
      back_outer_radius = peak_radius * 1.25992105; // A factor of 2 ^ (1/3) will make the background
      // shell volume equal to the peak region volume.
      std::pair <double, V3D> peak_q;
      for (size_t i = 0; i < n_peaks; i++) {
        V3D hkl(peaks[i].getH(), peaks[i].getK(), peaks[i].getL());
        if (Geometry::IndexingUtils::ValidIndex(hkl, 1.0)) {
          peak_q.first = 1.;
          peak_q.second = peaks[i].getQLabFrame();
          std::vector<double> axes_radii;
          integrator.ellipseIntegrateEvents(peak_q, specify_size, peak_radius,
                                            back_inner_radius, back_outer_radius,
                                            axes_radii, inti, sigi);
          peaks[i].setIntensity(inti);
          peaks[i].setSigmaIntensity(sigi);
          if (axes_radii.size() == 3){
            principalaxis1.push_back(axes_radii[0]);
            principalaxis2.push_back(axes_radii[1]);
            principalaxis3.push_back(axes_radii[2]);
          }
        } else {
          peaks[i].setIntensity(0.0);
          peaks[i].setSigmaIntensity(0.0);
        }
      }
      if (principalaxis1.size() > 1 ){
        size_t histogramNumber = 3;
        Workspace_sptr wsProfile2 = WorkspaceFactory::Instance().create(
            "Workspace2D", histogramNumber, principalaxis1.size(), principalaxis1.size());
        Workspace2D_sptr wsProfile2D2 = boost::dynamic_pointer_cast<Workspace2D>(wsProfile2);
        AnalysisDataService::Instance().addOrReplace("EllipsoidAxes_2ndPass", wsProfile2D2);
        for (size_t j = 0; j < principalaxis1.size(); j++) {
          wsProfile2D2->dataX(0)[j] = static_cast<double>(j);
          wsProfile2D2->dataY(0)[j] = principalaxis1[j];
          wsProfile2D2->dataE(0)[j] = std::sqrt(principalaxis1[j]);
          wsProfile2D2->dataX(1)[j] = static_cast<double>(j);
          wsProfile2D2->dataY(1)[j] = principalaxis2[j];
          wsProfile2D2->dataE(1)[j] = std::sqrt(principalaxis2[j]);
          wsProfile2D2->dataX(2)[j] = static_cast<double>(j);
          wsProfile2D2->dataY(2)[j] = principalaxis3[j];
          wsProfile2D2->dataE(2)[j] = std::sqrt(principalaxis3[j]);
        }
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
 * @brief IntegrateEllipsoids::initTargetWSDescr Initialize the output
 *        information for the MD conversion framework.
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

} // namespace MDEvents
} // namespace Mantid
