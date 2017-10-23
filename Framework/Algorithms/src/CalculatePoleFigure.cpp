#include "MantidAlgorithms/CalculatePoleFigure.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Run.h"
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
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "Result pole figure mapping Table");

  declareProperty("HROTName", "HROT", "Log name of HROT in input workspace");

  declareProperty("WHATEVERNAME", "WHATEVERT",
                  "Second name of some log for pole figure.");

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

  // Set up input data type
}

//----------------------------------------------------------------------------------------------
/** Process input properties
 */
void CalculatePoleFigure::processInputs() {

  // get inputs
  m_nameHROT = getPropertyValue("HROTName");
  m_nameOmega = getPropertyValue("Whatever");

  //
  m_inputWS = getProperty("InputWorkspace");

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

  // set property
  setProperty("OutputWorkspace", outputWS);
}

//----------------------------------------------------------------------------------------------
/** Calcualte pole figure
 */
void CalculatePoleFigure::calculatePoleFigure() {

  // get hrot and etc.
  TimeSeriesProperty<double> *hrotprop =
      dynamic_cast<TimeSeriesProperty<double> *>(
          m_inputWS->run().getProperty(m_nameHROT));
  double hrot = hrotprop->lastValue();

  // get source and positons
  Kernel::V3D srcpos = m_inputWS->getInstrument()->getSource()->getPos();
  Kernel::V3D samplepos = m_inputWS->getInstrument()->getSample()->getPos();
  Kernel::V3D k_sample_srcpos = samplepos - srcpos;

  std::vector<std::vector<double>> polefigurevector();

  for (size_t iws = 0; iws < m_inputWS->getNumberHistograms(); ++iws) {
    // get detector position
    Kernel::V3D detpos = m_inputWS->getInstrument()->getDetector(iws)->getPos();
    Kernel::V3D k_det_sample = detpos - samplepos;

    // calcualte pole figure position
    convertCoordinates();
  }
}

//----------------------------------------------------------------------------------------------
/** calculate peak intensity by simple integration without concerning the
 * background
 */
double CalculatePoleFigure::calculatePeakIntensitySimple(size_t iws,
                                                         double dmin,
                                                         double dmax) {

  // check
  if (iws >= m_inputWS->getNumberHistograms())
    throw std::runtime_error("Input workspace index exceeds input workspace's "
                             "number of histograms.");

  // integrate
  double intensity(0);

  // convert from dmin and dmax in double to index of X-axis
  auto vecx = m_inputWS->histogram(iws).x();
  auto vecy = m_inputWS->histogram(iws).y();
  size_t dmin_index = static_cast<size_t>(
      std::lower_bound(vecx.begin(), vecx.end(), dmin) - vecx.begin());
  size_t dmax_index = static_cast<size_t>(
      std::lower_bound(vecx.begin(), vecx.end(), dmax) - vecx.begin());

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

} // namespace Mantid
} // namespace Algorithms
