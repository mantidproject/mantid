#include <iostream>
#include <fstream>
#include <boost/math/special_functions/round.hpp>
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidDataObjects/Peak.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidMDEvents/MDTransfFactory.h"
#include "MantidMDEvents/UnitsConversionHelper.h"
#include "MantidMDEvents/Integrate3DEvents.h"
#include "MantidMDEvents/IntegrateEllipsoids.h"


using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace MDEvents {
/** NOTE: This has been adapted from the SaveIsawQvector algorithm.
 */

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(IntegrateEllipsoids)

/// This only works for diffraction.
const std::string ELASTIC("Elastic");

/// Only convert to Q-vector.
const std::string Q3D("Q3D");

/// Q-vector is always three dimensional.
const std::size_t DIMS(3);

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

  declareProperty(new WorkspaceProperty<EventWorkspace>(
                      "InputWorkspace", "", Direction::Input, ws_valid),
                  "An input EventWorkspace with time-of-flight units along "
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
}

//---------------------------------------------------------------------
/** Execute the algorithm.
 */
void IntegrateEllipsoids::exec() {
  // get the input workspace
  EventWorkspace_sptr wksp = getProperty("InputWorkspace");

  // error out if there are not events
  if (wksp->getNumberEvents() <= 0) {
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
  std::vector<V3D> hkl_vectors;
  for (size_t i = 0; i < n_peaks; i++) // Note: we skip un-indexed peaks
  {
    V3D hkl(peaks[i].getH(), peaks[i].getK(), peaks[i].getL());
    if (Geometry::IndexingUtils::ValidIndex(hkl, 1.0)) // use tolerance == 1 to
                                                       // just check for (0,0,0)
    {
      peak_q_list.push_back(V3D(peaks[i].getQLabFrame()));
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
  Integrate3DEvents integrator(peak_q_list, UBinv, radius);

  // get the events and add
  // them to the inegrator
  // set up a descripter of where we are going
  this->initTargetWSDescr(wksp);

  // units conersion helper
  UnitsConversionHelper unitConv;
  unitConv.initialize(m_targWSDescr, "Momentum");

  // initialize the MD coordinates conversion class
  MDTransf_sptr q_converter =
      MDTransfFactory::Instance().create(m_targWSDescr.AlgID);
  q_converter->initialize(m_targWSDescr);

  // set up the progress bar
  const size_t numSpectra = wksp->getNumberHistograms();
  Progress prog(this, 0.5, 1.0, numSpectra);

  // loop through the eventlists
  std::vector<double> buffer(DIMS);

  std::vector<V3D> event_qs;
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
    std::vector<coord_t> locCoord(DIMS, 0.);
    unitConv.updateConversion(i);
    q_converter->calcYDepCoordinates(locCoord, i);

    // loop over the events
    double signal(1.);  // ignorable garbage
    double errorSq(1.); // ignorable garbage
    const std::vector<WeightedEventNoTime> &raw_events =
        events.getWeightedEventsNoTime();
    event_qs.clear();
    for (auto event = raw_events.begin(); event != raw_events.end(); ++event) {
      double val = unitConv.convertUnits(event->tof());
      q_converter->calcMatrixCoord(val, locCoord, signal, errorSq);
      for (size_t dim = 0; dim < DIMS; ++dim) {
        buffer[dim] = locCoord[dim];
      }
      V3D q_vec(buffer[0], buffer[1], buffer[2]);
      event_qs.push_back(q_vec);
    } // end of loop over events in list

    integrator.addEvents(event_qs);

    prog.report();
  } // end of loop over spectra

  double inti;
  double sigi;
  for (size_t i = 0; i < n_peaks; i++) {
    V3D hkl(peaks[i].getH(), peaks[i].getK(), peaks[i].getL());
    if (Geometry::IndexingUtils::ValidIndex(hkl, 1.0)) {
      V3D peak_q(peaks[i].getQLabFrame());
      std::vector<double> axes_radii;
      auto shape = integrator.ellipseIntegrateEvents(peak_q, specify_size, peak_radius,
                                        back_inner_radius, back_outer_radius,
                                        axes_radii, inti, sigi);
      peaks[i].setIntensity(inti);
      peaks[i].setSigmaIntensity(sigi);
      peaks[i].setPeakShape(shape);
      if (axes_radii.size() == 3) {
        g_log.notice()
            << "Radii of three axes of ellipsoid for integrating peak " << i
            << " = ";
        for (int i3 = 0; i3 < 3; i3++) {
          g_log.notice() << axes_radii[i3] << "  ";
        }
        g_log.notice() << std::endl;
      }
    } else {
      peaks[i].setIntensity(0.0);
      peaks[i].setSigmaIntensity(0.0);
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
void IntegrateEllipsoids::initTargetWSDescr(EventWorkspace_sptr wksp) {
  m_targWSDescr.setMinMax(std::vector<double>(3, -2000.),
                          std::vector<double>(3, 2000.));
  m_targWSDescr.buildFromMatrixWS(wksp, Q3D, ELASTIC);
  m_targWSDescr.setLorentsCorr(false);

  // generate the detectors table
  Mantid::API::Algorithm_sptr childAlg =
      createChildAlgorithm("PreprocessDetectorsToMD", 0., .5);
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
