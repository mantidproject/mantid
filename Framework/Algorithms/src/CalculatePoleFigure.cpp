#include "MantidAlgorithms/CalculatePoleFigure.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
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

#include <H5Cpp.h>

namespace Mantid {
namespace Algorithms {

using std::string;
using namespace HistogramData;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalculatePoleFigure)

using namespace Mantid::Kernel;
using namespace Mantid::API;

const std::string CalculatePoleFigure::name() const {
  return "CalculatePoleFigure";
}

int CalculatePoleFigure::version() const { return 1; }

const std::string CalculatePoleFigure::category() const {
  return "Diffraction\\Utility";
}

/** Initialize the algorithm's properties.
*/
void CalculatePoleFigure::init() {
  auto uv = boost::make_shared<API::WorkspaceUnitValidator>("dSpacing");

  declareProperty(make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                                   Direction::Input, uv),
                  "Name of input workspace to calculate Pole Figure from.");
  declareProperty(make_unique<WorkspaceProperty<API::ITableWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Result pole figure mapping Table");

  declareProperty("HROTName", "BL7:Mot:Parker:HROT.RBV",
                  "Log name of HROT in input workspace");

  declareProperty("OmegaName", "BL7:Mot:Sample:Omega.RBV",
                  "Log name of Omega for pole figure.");

  declareProperty(make_unique<Kernel::ArrayProperty<double>>("PoleFigure"),
                  "Output 2D vector for calcualte pole figure.");

  declareProperty(make_unique<API::FileProperty>(
                      "PoleFigureFile", "", API::FileProperty::OptionalSave),
                  "Name of optional output file for pole figure.");

  declareProperty("MinD", EMPTY_DBL(), "Lower boundary of peak in dSpacing.");
  declareProperty("MaxD", EMPTY_DBL(), "Upper boundary of peak in dSpacing.");

  std::vector<std::string> peakcaloptions;
  peakcaloptions.push_back("SimpleIntegration");
  // peakcaloptions.push_back("Fit:Peak+Background");
  declareProperty("PeakIntensityCalculation", "SimpleIntegration",
                  "Algorithm type to calcualte the peak intensity.");

  // output vectors
  declareProperty(Kernel::make_unique<ArrayProperty<double>>(
                      "R_TD", Direction::Output),
                  "Array for R_TD");
  declareProperty(Kernel::make_unique<ArrayProperty<double>>(
                      "R_ND", Direction::Output),
                  "Array for R_ND");
  declareProperty(Kernel::make_unique<ArrayProperty<double>>(
                      "PeakIntensity", Direction::Output),
                  "Array for peak intensities");


  // Set up input data type
}

//----------------------------------------------------------------------------------------------
/** Process input properties
 */
void CalculatePoleFigure::processInputs() {

  // get inputs
  m_nameHROT = getPropertyValue("HROTName");
  m_nameOmega = getPropertyValue("OmegaName");

  //
  m_inputWS = getProperty("InputWorkspace");

  // check peak range
  double dmin = getProperty("MinD");
  double dmax = getProperty("MaxD");
  if (isEmpty(dmin) || isEmpty(dmax))
    throw std::invalid_argument("Peak range (dmin and dmax) must be given!");

  // check whether the log exists
  auto hrot = m_inputWS->run().getProperty(m_nameHROT);
  if (!hrot) {
    throw std::invalid_argument("HROT does not exist in sample log.");
  }
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
*/
void CalculatePoleFigure::exec() {
  // get input data
  processInputs();

  // calcualte pole figure
  calculatePoleFigure();

  // construct output
  generateOutputs();
}

//----------------------------------------------------------------------------------------------
/** Calcualte pole figure
 */
void CalculatePoleFigure::calculatePoleFigure() {
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

  // TODO/NEXT - After unit test and user test are passed. try to parallelize this loop by openMP
  for (size_t iws = 0; iws < m_inputWS->getNumberHistograms(); ++iws) {
    // get detector position
    Kernel::V3D detpos = m_inputWS->getInstrument()->getDetector(static_cast<detid_t>(iws))->getPos();
    Kernel::V3D k_det_sample = detpos - samplepos;
    Kernel::V3D k_det_sample_unit = k_det_sample / k_det_sample.norm();
    Kernel::V3D qvector = k_sample_src_unit - k_det_sample_unit;
    Kernel::V3D unit_q = qvector / qvector.norm();

    // calcualte pole figure position
    double r_td, r_nd;
    convertCoordinates(unit_q, hrot, omega, r_td, r_nd);

    // calcualte peak intensity
    double peak_intensity_i = calculatePeakIntensitySimple(iws, m_peakDRange);

    // set up value
    m_poleFigureRTDVector[iws] = r_td;
    m_poleFigureRNDVector[iws] = r_nd;
    m_poleFigurePeakIntensityVector[iws] = peak_intensity_i;
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** calculate peak intensity by simple integration without concerning the
 * background
 */
double CalculatePoleFigure::calculatePeakIntensitySimple(
    size_t iws, const std::pair<double, double> &peak_range) {

  // check
  if (iws >= m_inputWS->getNumberHistograms())
    throw std::runtime_error("Input workspace index exceeds input workspace's "
                             "number of histograms.");
  double d_min = peak_range.first;
  double d_max = peak_range.second;

  // integrate
  double intensity(0);

  // convert from dmin and dmax in double to index of X-axis
  auto vecx = m_inputWS->histogram(iws).x();
  auto vecy = m_inputWS->histogram(iws).y();
  size_t dmin_index = static_cast<size_t>(
      std::lower_bound(vecx.begin(), vecx.end(), d_min) - vecx.begin());
  size_t dmax_index = static_cast<size_t>(
      std::lower_bound(vecx.begin(), vecx.end(), d_max) - vecx.begin());

  if (dmax_index <= dmin_index)
    throw std::runtime_error("dmin and dmax either not in order or exceeds the "
                             "range of vector of X.");
  if (dmax_index >= vecy.size())
    dmax_index = vecy.size() - 1;

  for (auto d_index = dmin_index; d_index < dmax_index; ++d_index) {
    double y_i = vecy[d_index];
    double d_x = vecx[d_index] - vecx[d_index - 1];
    intensity += y_i * d_x;
  }

  return intensity;
}

//----------------------------------------------------------------------------------------------
/** generate output workspaces
 * @brief CalculatePoleFigure::generateOutputs
 */
void CalculatePoleFigure::generateOutputs() {
  // create output TableWorkspace
  API::ITableWorkspace_sptr output_table =
      boost::make_shared<DataObjects::TableWorkspace>();
  output_table->addColumn("int", "WorkspaceIndex");
  output_table->addColumn("double", "R_TD");
  output_table->addColumn("double", "R_ND");
  output_table->addColumn("double", "Intensity");

  // add values
  for (size_t iws = 0; iws < m_poleFigurePeakIntensityVector.size(); ++iws) {
    API::TableRow row_i = output_table->appendRow();
    row_i << static_cast<int>(iws) << m_poleFigureRTDVector[iws]
          << m_poleFigureRNDVector[iws] << m_poleFigurePeakIntensityVector[iws];
  }

  // set properties for output
  setProperty("OutputWorkspace", output_table);

  setProperty("R_TD", m_poleFigureRTDVector);
  setProperty("R_ND", m_poleFigureRNDVector);
  setProperty("PeakIntensity", m_poleFigurePeakIntensityVector);

}

//----------------------------------------------------------------------------------------------
/** convert from Q vector to R_TD and R_ND
 * @brief CalculatePoleFigure::convertCoordinates
 * @param unitQ
 * @param hrot
 * @param omega
 * @param r_td
 * @param r_nd
 */
void CalculatePoleFigure::convertCoordinates(Kernel::V3D unitQ,
                                             const double &hrot,
                                             const double &omega, double &r_td,
                                             double &r_nd) {
  // define constants
  const double psi = -45.;
  const double phi = 0;

  double omega_prime = omega - psi + 135.;
  double tau_pp = -hrot - phi;

  //
  double omega_prim_rad = omega_prime * M_PI / 180.;
  double tau_pp_rad = tau_pp * M_PI / 180.;

  // calculate first rotation
  Kernel::V3D k1(0, 1, 0);
  Kernel::V3D part1 = unitQ * cos(omega_prim_rad);
  Kernel::V3D part2 = (k1.cross_prod(unitQ)) * sin(omega_prim_rad);
  Kernel::V3D part3 =
      k1 * ((1 - cos(omega_prim_rad)) * (k1.scalar_prod(unitQ)));
  Kernel::V3D unitQPrime = part1 + part2 + part3;

  // calcualte second rotation
  Kernel::V3D k2(0, 0, 1);
  part1 = unitQPrime * cos(tau_pp_rad);
  part2 = (k2.cross_prod(unitQPrime)) * sin(tau_pp_rad);
  part3 = k2 * (k2.scalar_prod(unitQPrime) * (1-cos(tau_pp_rad)));
  Kernel::V3D unitQpp = part1 + part2 + part3;

  // project to the pole figure
  double sign(1);
  if (unitQpp.Z() < 0)
    sign = -1;

  r_td = unitQpp.Y() * sign;
  r_nd = - unitQpp.X() * sign;

  return;
}

// TODO/ISSUE/NOW - Implement HDF5 output by H5Util
// ... ...

} // namespace Mantid
} // namespace Algorithms
