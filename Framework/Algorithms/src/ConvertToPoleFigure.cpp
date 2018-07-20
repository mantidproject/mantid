#include "MantidAlgorithms/ConvertToPoleFigure.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/MDEvent.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventInserter.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/PhysicalConstants.h"
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

  declareProperty(
      make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
          "IntegratedPeakIntensityWorkspace", "", Direction::Input, uv),
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
  m_countWS = getProperty("IntegratedPeakIntensityWorkspace");

  // check
  if (m_inputWS->getNumberHistograms() != m_countWS->getNumberHistograms()) {
    // TODO - A better output error message
    throw std::runtime_error("");
  }

  // get inputs
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

  return;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
*/
void ConvertToPoleFigure::exec() {
  // get input data
  processInputs();

  // calcualte pole figure
  convertToPoleFigure();

  // construct output
  generateOutputs();
}

//----------------------------------------------------------------------------------------------
/** Calcualte pole figure
 */
void ConvertToPoleFigure::convertToPoleFigure() {
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
  Kernel::V3D srcpos = m_inputWS->getInstrument()->getSource()->getPos();
  Kernel::V3D samplepos = m_inputWS->getInstrument()->getSample()->getPos();
  Kernel::V3D k_sample_srcpos = samplepos - srcpos;
  Kernel::V3D k_sample_src_unit = k_sample_srcpos / k_sample_srcpos.norm();

  // TODO/NEXT - After unit test and user test are passed. try to parallelize
  // this loop by openMP
  for (size_t iws = 0; iws < m_inputWS->getNumberHistograms(); ++iws) {
    // get detector position
    auto detector = m_inputWS->getDetector(iws);
    Kernel::V3D detpos = detector->getPos();
    Kernel::V3D k_det_sample = detpos - samplepos;
    Kernel::V3D k_det_sample_unit = k_det_sample / k_det_sample.norm();
    Kernel::V3D qvector = k_sample_src_unit - k_det_sample_unit;
    Kernel::V3D unit_q = qvector / qvector.norm();

    // calcualte pole figure position
    double r_td, r_nd;
    rotateVectorQ(unit_q, hrot, omega, r_td, r_nd);

    // get peak intensity
    double peak_intensity_i = m_countWS->histogram(iws).y()[0];

    // set up value
    m_poleFigureRTDVector[iws] = r_td;
    m_poleFigureRNDVector[iws] = r_nd;
    m_poleFigurePeakIntensityVector[iws] = peak_intensity_i;
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** generate output workspaces
 * @brief ConvertToPoleFigure::generateOutputs
 */
void ConvertToPoleFigure::generateOutputs() {
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
 * @param unitQ
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

  g_log.notice() << "Input: omega = " << omega << ", " << "HROT = " << hrot << "\n";
  g_log.notice() << "Q = " << unitQ.toString() << "\n";

  //
  double omega_prim_rad = omega_prime * M_PI / 180.;
  double tau_pp_rad = tau_pp * M_PI / 180.;

  // calculate first rotation
  Kernel::V3D k1(0, 1, 0);
  Kernel::V3D part1 = unitQ * cos(omega_prim_rad);
  g_log.notice() << "Part 1: " << part1.toString() << "\n";

  Kernel::V3D part2 = (k1.cross_prod(unitQ)) * sin(omega_prim_rad);
  g_log.notice() << "Part 2: " << part1.toString() << "\n";

  Kernel::V3D part3 =
      k1 * ((1 - cos(omega_prim_rad)) * (k1.scalar_prod(unitQ)));
  Kernel::V3D unitQPrime = part1 + part2 + part3;

  g_log.notice() << "Q' = " << unitQPrime.toString() << "\n";

  // calcualte second rotation
  Kernel::V3D k2(0, 0, 1);
  part1 = unitQPrime * cos(tau_pp_rad);
  part2 = (k2.cross_prod(unitQPrime)) * sin(tau_pp_rad);
  part3 = k2 * (k2.scalar_prod(unitQPrime) * (1 - cos(tau_pp_rad)));
  Kernel::V3D unitQpp = part1 + part2 + part3;

  // project to the pole figure by point light
  double sign(1);
  if (unitQpp.Z() < 0)
    sign = -1;

  double factor = 1. + sign * unitQpp.Z();
  r_td = unitQpp.Y() * sign * 2. / factor;
  r_nd = -unitQpp.X() * sign * 2. / factor;

  return;
}

} // namespace Mantid
} // namespace Algorithms
