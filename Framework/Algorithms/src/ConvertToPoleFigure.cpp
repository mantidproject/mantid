#include "MantidAlgorithms/ConvertToPoleFigure.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/MDEvent.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventInserter.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include <cmath>
#include <sstream>

// #include <H5Cpp.h>

namespace Mantid {
namespace Algorithms {

using std::string;
using namespace HistogramData;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertToPoleFigure)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

const std::string ConvertToPoleFigure::name() const {
  return "ConvertToPoleFigure";
}

int ConvertToPoleFigure::version() const { return 1; }

const std::string ConvertToPoleFigure::category() const {
  return "Diffraction\\Utility";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
*/
void ConvertToPoleFigure::init() {
  auto uv = boost::make_shared<API::WorkspaceUnitValidator>("dSpacing");

  declareProperty(make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                                   Direction::Input, uv),
                  "Name of input workspace containing peak intensity and "
                  "instrument information.");

  declareProperty(make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "IntegratedPeakIntensityWorkspace", "", Direction::Input,
                      API::PropertyMode::Optional, uv),
                  "Name of the output MatrixWorkspace containing the events' "
                  "counts in the ROI for each spectrum.");

  declareProperty("HROTName", "BL7:Mot:Parker:HROT.RBV",
                  "Log name of HROT in input workspace");

  declareProperty("OmegaName", "BL7:Mot:Sample:Omega.RBV",
                  "Log name of Omega for pole figure.");

  // output MD workspace
  declareProperty(
      make_unique<WorkspaceProperty<API::IMDEventWorkspace>>(
          "OutputWorkspace", "", Direction::Output),
      "Name of the output IEventWorkspace containing pole figure information.");

  declareProperty("EventMode", true,
                  "If True, then the pole figure will be based on each event;"
                  " and IntegratedPeakIntensityWorkspace is not required. "
                  "Otherwise, the pole figure "
                  " will be calculated base on each spectra (hitogram mode) "
                  "and IntegratedPeakIntensityWorkspace "
                  " is required.");

  declareProperty("MinD", EMPTY_DBL(),
                  "Lower boundary of the peak range in dSpacing"
                  "It must be specified in the EventMode.");
  declareProperty("MaxD", EMPTY_DBL(),
                  "Upper boundary of the peak range in dSpacing"
                  "It must be specified in the EventMode");

  // output vectors
  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>("R_TD", Direction::Output),
      "Array for R_TD.");
  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>("R_ND", Direction::Output),
      "Array for R_ND.");
  declareProperty(Kernel::make_unique<ArrayProperty<double>>("PeakIntensity",
                                                             Direction::Output),
                  "Array for peak intensites.");

  return;
}

//----------------------------------------------------------------------------------------------
/** Process input properties
 */
void ConvertToPoleFigure::processInputs() {
  // get input workspaces
  m_inputWS = getProperty("InputWorkspace");

  // get inputs for HROT and Omega
  m_nameHROT = getPropertyValue("HROTName");
  m_nameOmega = getPropertyValue("OmegaName");
  // check whether the log exists
  auto hrot = m_inputWS->run().getProperty(m_nameHROT);
  if (!hrot) {
    throw std::invalid_argument("HROT does not exist in sample log.");
  }
  auto omega = m_inputWS->run().getProperty(m_nameOmega);
  if (!omega) {
    throw std::invalid_argument("Omega does not exist in sample log!");
  }

  // event mode or histogram mode
  m_inEventMode = getProperty("EventMode");
  if (m_inEventMode) {
    processEventModeInputs();

  } else {
    // histgram mode
    // get the event count workspace
    std::string peakwsname =
        getPropertyValue("IntegratedPeakIntensityWorkspace");
    if (peakwsname.size() == 0) {
      // empty due to not given
      std::stringstream errss;
      errss << "In histogram mode, IntegratedPeakIntensityWorkspace must be "
               "given.";
      g_log.error(errss.str());
      throw std::invalid_argument(errss.str());
    }
    m_peakIntensityWS = getProperty("IntegratedPeakIntensityWorkspace");

    // check
    if (m_inputWS->getNumberHistograms() !=
        m_peakIntensityWS->getNumberHistograms()) {
      // histogram mode
      throw std::runtime_error("");
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** process the input arguments for event mode
 * @brief ConvertToPoleFigure::processEventModeInputs
 */
void ConvertToPoleFigure::processEventModeInputs() {
  // check whether the input workspace is an EventWorkspace
  m_eventWS =
      boost::dynamic_pointer_cast<const DataObjects::EventWorkspace>(m_inputWS);
  if (!m_eventWS) {
    std::stringstream errss;
    errss << "Input MatrixWorkspace " << m_inputWS->getName()
          << " is not an EventWorkspace";
    g_log.notice(errss.str());
    throw std::invalid_argument(errss.str());
  }

  // parsing d-spacing range
  double min_d = getProperty("MinD");
  double max_d = getProperty("MaxD");
  g_log.notice() << "[DB...BAT] EventWorkspace range: "
                 << m_eventWS->getTofMin() << ", " << m_eventWS->getTofMax()
                 << "\n";
  if (isEmpty(min_d) || isEmpty(max_d)) {
    // minD or maxD is not specified
    throw std::invalid_argument("Both MinD and MaxD shall be specified.");
  } else if (min_d >= max_d) {
    // value not in order
    std::stringstream errss;
    errss << "MinD " << min_d << " cannot be larger than or equal to MaxD "
          << max_d;
    g_log.notice(errss.str());
    throw std::invalid_argument(errss.str());
  } else if (min_d < m_eventWS->getTofMin() || max_d > m_eventWS->getTofMax()) {
    // value out of range
    std::stringstream errss;
    errss << "MinD " << min_d << " and MaxD" << max_d
          << " cannot be out of input EventWorkspace "
          << "'s dSpacing range: " << m_eventWS->getTofMin() << ", "
          << m_eventWS->getTofMax();
    g_log.notice(errss.str());
    throw std::invalid_argument(errss.str());
  } else {
    m_minD = min_d;
    m_maxD = max_d;
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
    */
void ConvertToPoleFigure::exec() {
  // get input data
  processInputs();

  if (m_inEventMode) {
    convertToPoleFigureEventMode();

    // initialize output (this is fake) TODO - It is for try-error-develop only
    m_poleFigureRTDVector.resize(m_inputWS->getNumberHistograms());
    m_poleFigureRNDVector.resize(m_inputWS->getNumberHistograms());
    m_poleFigurePeakIntensityVector.resize(m_inputWS->getNumberHistograms());
    generateOutputsHistogramMode();

  } else {
    // calcualte pole figure
    convertToPoleFigureHistogramMode();

    // construct output
    generateOutputsHistogramMode();
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** calculate pole figure in the event mode
 * @brief ConvertToPoleFigure::convertToPoleFigureEventMode
 */
void ConvertToPoleFigure::convertToPoleFigureEventMode() {
  // initialize output:
  setupOutputVectors();

  // sort events
  sortEventWS();

  // get hrot and omega series
  TimeSeriesProperty<double> *hrotprop =
      dynamic_cast<TimeSeriesProperty<double> *>(
          m_inputWS->run().getProperty(m_nameHROT));

  TimeSeriesProperty<double> *omegaprop =
      dynamic_cast<TimeSeriesProperty<double> *>(
          m_inputWS->run().getProperty(m_nameOmega));

  // get instrument information
  Kernel::V3D sample_pos, sample_src_unit_k;
  retrieveInstrumentInfo(sample_pos, sample_src_unit_k);

  // go through spectra
  // TODO / NOW - this loop shall be turned to OpenMP to TEST SPEED
  // TODO - Need statistic on the number of events falling in range
  size_t num_spectra = m_eventWS->getNumberHistograms();
  for (size_t iws = 0; iws < num_spectra; ++iws) {
    auto events = m_eventWS->getSpectrum(iws).getEvents();
    if (events.size() == 0)
      continue;
    g_log.notice() << "[DB...INFO] working on spectrum " << iws << "\n";
    Kernel::V3D unit_vec_q = calculateUnitQ(iws, sample_pos, sample_src_unit_k);
    size_t index_min_d = findDRangeInEventList(events, m_minD, false);
    size_t index_max_d = findDRangeInEventList(events, m_maxD, true);
    g_log.notice() << "[DB...INFO] spectrum " << iws
                   << " in range: " << index_min_d << " --- " << index_max_d
                   << "\n";
    // TODO create a vector for rotated Q
    for (size_t iev = index_min_d; iev < index_max_d; ++iev) {
      // get event's wall time
      TofEvent event_i = events[iev];
      Types::Core::DateAndTime pulsetime = event_i.pulseTime();
      double tof = event_i.tof();
      int64_t tof_ns = static_cast<int64_t>(tof * 1000); // FIXME - really 1000?
      pulsetime += tof_ns;
      // get hrot and omega
      double hrot_i = hrotprop->getSingleValue(pulsetime);
      double omega_i = omegaprop->getSingleValue(pulsetime);
      // rotation q
      double r_td_i;
      double r_nd_i;
      rotateVectorQ(unit_vec_q, hrot_i, omega_i, r_td_i, r_nd_i);
      // save.
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** For event mode, it is not pratical to output all the MDEvents. It is
 * required some experiment
 * to determine how to export the pole figure to python script. And how python
 * script can hanle
 * the data size in a reasonable time
 * @brief ConvertToPoleFigure::setupOutputVectors
 */
void ConvertToPoleFigure::setupOutputVectors() {
  // TODO - TO be determined how to deal with this
}

//----------------------------------------------------------------------------------------------
/**  Examine whether any spectrum does not have detector
 * Warning message will be written out
 * @brief FilterEvents::examineEventWS
 */
void ConvertToPoleFigure::sortEventWS() {
  // sort events
  DataObjects::EventSortType sortType = DataObjects::TOF_SORT;

  // This runs the SortEvents algorithm in parallel
  m_eventWS->sortAll(sortType, nullptr);

  return;
}

//----------------------------------------------------------------------------------------------
/** Calcualte pole figure
 */
void ConvertToPoleFigure::convertToPoleFigureHistogramMode() {
  // initialize output
  m_poleFigureRTDVector.resize(m_inputWS->getNumberHistograms());
  m_poleFigureRNDVector.resize(m_inputWS->getNumberHistograms());
  m_poleFigurePeakIntensityVector.resize(m_inputWS->getNumberHistograms());

  // get hrot and etc.
  TimeSeriesProperty<double> *hrotprop =
      dynamic_cast<TimeSeriesProperty<double> *>(
          m_inputWS->run().getProperty(m_nameHROT));
  double hrot = hrotprop->lastValue();

  TimeSeriesProperty<double> *omegaprop =
      dynamic_cast<TimeSeriesProperty<double> *>(
          m_inputWS->run().getProperty(m_nameOmega));
  double omega = omegaprop->lastValue();

  // get source and positons
  //  Kernel::V3D srcpos = m_inputWS->getInstrument()->getSource()->getPos();
  //  Kernel::V3D samplepos = m_inputWS->getInstrument()->getSample()->getPos();
  //  Kernel::V3D k_sample_srcpos = samplepos - srcpos;
  //  Kernel::V3D k_sample_src_unit = k_sample_srcpos / k_sample_srcpos.norm();
  Kernel::V3D samplepos, k_sample_src_unit;
  retrieveInstrumentInfo(samplepos, k_sample_src_unit);

  // TODO/NEXT - After unit test and user test are passed. try to
  // parallelize
  // this loop by openMP
  for (size_t iws = 0; iws < m_inputWS->getNumberHistograms(); ++iws) {
    // get detector position
    //    auto detector = m_inputWS->getDetector(iws);
    //    Kernel::V3D detpos = detector->getPos();
    //    Kernel::V3D k_det_sample = detpos - samplepos;
    //    Kernel::V3D k_det_sample_unit = k_det_sample / k_det_sample.norm();
    //    Kernel::V3D qvector = k_det_sample_unit - k_sample_src_unit;
    //    Kernel::V3D unit_q = qvector / qvector.norm();
    Kernel::V3D unit_q = calculateUnitQ(iws, samplepos, k_sample_src_unit);

    // calcualte pole figure position
    double r_td, r_nd;
    rotateVectorQ(unit_q, hrot, omega, r_td, r_nd);

    // get peak intensity
    double peak_intensity_i = m_peakIntensityWS->histogram(iws).y()[0];

    // set up value
    m_poleFigureRTDVector[iws] = r_td;
    m_poleFigureRNDVector[iws] = r_nd;
    m_poleFigurePeakIntensityVector[iws] = peak_intensity_i;
  }

  return;
}

void ConvertToPoleFigure::retrieveInstrumentInfo(
    Kernel::V3D &sample_pos, Kernel::V3D &sample_src_unit_k) {
  Kernel::V3D srcpos = m_inputWS->getInstrument()->getSource()->getPos();
  sample_pos = m_inputWS->getInstrument()->getSample()->getPos();
  Kernel::V3D k_sample_srcpos = sample_pos - srcpos;
  sample_src_unit_k = k_sample_srcpos / k_sample_srcpos.norm();

  return;
}

//----------------------------------------------------------------------------------------------
/** calculate the unit Q value from a detector
 * @brief calculateUnitQ
 * @param iws :: workspace index
 * @param samplepos :: sample position
 * @param k_sample_src_unit:: unit k vector from source to sample
 * @return
 */
Kernel::V3D ConvertToPoleFigure::calculateUnitQ(size_t iws, V3D samplepos,
                                                V3D k_sample_src_unit) {
  auto detector = m_inputWS->getDetector(iws);
  Kernel::V3D detpos = detector->getPos();
  Kernel::V3D k_det_sample = detpos - samplepos;
  Kernel::V3D k_det_sample_unit = k_det_sample / k_det_sample.norm();
  Kernel::V3D qvector = k_det_sample_unit - k_sample_src_unit;
  Kernel::V3D unit_q = qvector / qvector.norm();

  return unit_q;
}

//----------------------------------------------------------------------------------------------
/** generate output workspaces
 * @brief ConvertToPoleFigure::generateOutputs
 */
void ConvertToPoleFigure::generateOutputsHistogramMode() {
  // create MDEventWorkspace
  // Create a target output workspace.
  // 2D as (x, y) signal error
  // NOTE: the template is from ImportMDEventWorkspace Line 271 and so on
  API::IMDEventWorkspace_sptr out_event_ws =
      DataObjects::MDEventFactory::CreateMDWorkspace(2, "MDEvent");
  // x-value range and y-value range
  std::vector<double> extentMins(2);
  std::vector<double> extentMaxs(2);

  //  auto unitFactory = makeMDUnitFactoryChain();
  //  std::string units("U");
  //  auto mdUnit = unitFactory->create(units);

  // Set up X and Y dimension
  // Extract Dimensions and add to the output workspace.
  std::string xid("x");
  std::string xname("X");
  int xnbins = 100; // FIXME - a better number
  size_t dimx = 0;
  Mantid::Geometry::GeneralFrame xframe(
      Mantid::Geometry::GeneralFrame::GeneralFrameName, "U");
  out_event_ws->addDimension(
      Geometry::MDHistoDimension_sptr(new Geometry::MDHistoDimension(
          xid, xname, xframe, static_cast<coord_t>(extentMins[dimx]),
          static_cast<coord_t>(extentMaxs[dimx]), xnbins)));

  std::string yid("y");
  std::string yname("Y");
  int ynbins = 100; // FIXME - a better number
  size_t dimy = 1;
  Mantid::Geometry::GeneralFrame yframe(
      Mantid::Geometry::GeneralFrame::GeneralFrameName, "U");
  out_event_ws->addDimension(
      Geometry::MDHistoDimension_sptr(new Geometry::MDHistoDimension(
          yid, yname, yframe, static_cast<coord_t>(extentMins[dimy]),
          static_cast<coord_t>(extentMaxs[dimy]), ynbins)));

  // add MDEvents
  // Creates a new instance of the MDEventInserter.
  MDEventWorkspace<MDEvent<2>, 2>::sptr MDEW_MDEVENT_2 =
      boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<2>, 2>>(
          out_event_ws);

  MDEventInserter<MDEventWorkspace<MDEvent<2>, 2>::sptr> inserter(
      MDEW_MDEVENT_2);

  size_t num_md_events = m_poleFigureRNDVector.size();
  uint16_t detid = 0;
  uint16_t run_number = 1;
  for (size_t ievt = 0; ievt < num_md_events; ++ievt) {
    Mantid::coord_t data[2] = {static_cast<float>(m_poleFigureRTDVector[ievt]),
                               static_cast<float>(m_poleFigureRNDVector[ievt])};
    float signal = static_cast<float>(m_poleFigurePeakIntensityVector[ievt]);
    if (signal <= 0)
      throw std::runtime_error("Found negative signal");
    float error = std::sqrt(signal);
    inserter.insertMDEvent(signal, error * error, run_number, detid, data);
  }

  // set properties for output
  setProperty("OutputWorkspace", out_event_ws);
  setProperty("R_TD", m_poleFigureRTDVector);
  setProperty("R_ND", m_poleFigureRNDVector);
  setProperty("PeakIntensity", m_poleFigurePeakIntensityVector);
}

//----------------------------------------------------------------------------------------------
/** rotate a vector Q to sample coordinate and project to R_TD and R_ND
 * @brief ConvertToPoleFigure::convertCoordinates
 * @param unitQ: unit Q value for each individual pixel/detector
 * @param hrot
 * @param omega
 * @param r_td
 * @param r_nd
 */
void ConvertToPoleFigure::rotateVectorQ(Kernel::V3D unitQ, const double &hrot,
                                        const double &omega, double &r_td,
                                        double &r_nd) {
  // define constants
  const double psi = -45.;
  const double phi = 0;

  double omega_prime = omega - psi + 135.;
  double tau_pp = -hrot - phi;

  //  g_log.notice() << "\nQ = " << unitQ.toString() << "\n";
  //  g_log.notice() << "Input: omega = " << omega << ", "
  //                 << "HROT = " << hrot << "\n";

  //
  double omega_prim_rad = omega_prime * M_PI / 180.;
  double tau_pp_rad = tau_pp * M_PI / 180.;

  // calculate first rotation
  Kernel::V3D k1(0, 1, 0);
  Kernel::V3D part1 = unitQ * cos(omega_prim_rad);
  //  g_log.notice() << "Part 1: " << part1.toString() << "\n";

  Kernel::V3D part2 = (k1.cross_prod(unitQ)) * sin(omega_prim_rad);
  //  g_log.notice() << "Part 2: " << part1.toString() << "\n";

  Kernel::V3D part3 =
      k1 * ((1 - cos(omega_prim_rad)) * (k1.scalar_prod(unitQ)));
  Kernel::V3D unitQPrime = part1 + part2 + part3;

  //  g_log.notice() << "Q' = " << unitQPrime.toString() << "\n";

  // calcualte second rotation
  Kernel::V3D k2(0, 0, 1);
  part1 = unitQPrime * cos(tau_pp_rad);
  part2 = (k2.cross_prod(unitQPrime)) * sin(tau_pp_rad);
  part3 = k2 * (k2.scalar_prod(unitQPrime) * (1 - cos(tau_pp_rad)));
  Kernel::V3D unitQpp = part1 + part2 + part3;

  //  g_log.notice() << "Q'' = " << unitQpp.toString() << "\n";

  // project to the pole figure by point light
  double sign(1);
  // Qz with positive and negative zero will render to opposite result
  // though
  // physically
  // they are same.  Here in order to make the output consistent with
  // considerting numerical
  // resolution, positive is set to be larger than -1E-10
  if (unitQpp.Z() < -1.E-10)
    sign = -1;

  double factor = 1. + sign * unitQpp.Z();
  r_td = unitQpp.Y() * sign * 2. / factor;
  r_nd = -unitQpp.X() * sign * 2. / factor;

  return;
}

//----------------------------------------------------------------------------------------------
/**
 */
/** find a TOF value's nearest index in a sorted (by tof) vector
 * a binary search will be used
 * @brief ConvertToPoleFigure::findDRangeInEventList
 * @param vector
 * @param d_value
 * @param index_to_right :: if true, then the return index is equal to just
 * larger than the d-value; otherwise, the returned index is equal to just
 * smaller than d-value
 * @return
 */
size_t ConvertToPoleFigure::findDRangeInEventList(
    std::vector<Types::Event::TofEvent> vector, double d_value,
    bool index_to_right) {

  size_t left_index = 0;
  size_t right_index = vector.size() - 1; // included
  bool search(false);
  // extreme condition
  size_t index = 0;
  if (d_value < vector.front().tof()) {
    // d is too small
    index = 0;
  } else if (d_value > vector.back().tof()) {
    // d is too large
    index = vector.size() - 1;
  } else {
    // need to search
    search = true;
  }

  while (search) {
    if (left_index + 1 >= right_index) {
      // terminate condition such that TOF value resides within left index and
      // right index
      search = false;
      if (index_to_right)
        index = right_index;
      else
        index = left_index;
    } else {
      size_t mid_point_index = (left_index + right_index) / 2;
      if (d_value < vector[left_index].tof()) {
        right_index = mid_point_index;
      } else {
        left_index = mid_point_index;
      }
    }
  } // while

  return index;
}

} // namespace Mantid
} // namespace Algorithms
