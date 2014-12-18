#include "MantidAlgorithms/CalculateResolution.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <cmath>

#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace Algorithms {

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalculateResolution)

//----------------------------------------------------------------------------------------------
/** Constructor
*/
CalculateResolution::CalculateResolution() {}

//----------------------------------------------------------------------------------------------
/** Destructor
*/
CalculateResolution::~CalculateResolution() {}

//----------------------------------------------------------------------------------------------

/// Algorithm's name for identification. @see Algorithm::name
const std::string CalculateResolution::name() const {
  return "CalculateResolution";
};

/// Algorithm's version for identification. @see Algorithm::version
int CalculateResolution::version() const { return 1; };

/// Algorithm's category for identification. @see Algorithm::category
const std::string CalculateResolution::category() const {
  return "Reflectometry\\ISIS";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CalculateResolution::summary() const {
  return "Calculates the reflectometry resolution (dQ/Q) for a given "
         "workspace.";
};

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
*/
void CalculateResolution::init() {
  declareProperty(
      new WorkspaceProperty<>("Workspace", "", Direction::Input,
                              boost::make_shared<InstrumentValidator>()),
      "Workspace to calculate the instrument resolution of.");

  declareProperty("TwoTheta", Mantid::EMPTY_DBL(),
                  "Two theta scattering angle in degrees.");
  declareProperty("FirstSlitName", "slit1",
                  "Component name of the first slit.");
  declareProperty("SecondSlitName", "slit2",
                  "Component name of the second slit.");
  declareProperty("VerticalGapParameter", "vertical gap",
                  "Parameter the vertical gap of each slit can be found in.");
  declareProperty("TwoThetaLogName", "Theta",
                  "Name two theta can be found in the run log as.");

  declareProperty("Resolution", Mantid::EMPTY_DBL(),
                  "Calculated resolution (dq/q).", Direction::Output);
  declareProperty("TwoThetaOut", Mantid::EMPTY_DBL(),
                  "Two theta scattering angle in degrees.", Direction::Output);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
*/
void CalculateResolution::exec() {
  const MatrixWorkspace_sptr ws = getProperty("Workspace");
  double twoTheta = getProperty("TwoTheta");
  const std::string slit1Name = getProperty("FirstSlitName");
  const std::string slit2Name = getProperty("SecondSlitName");
  const std::string vGapParam = getProperty("VerticalGapParameter");
  const std::string twoThetaLogName = getProperty("TwoThetaLogName");

  if (isEmpty(twoTheta)) {
    const Kernel::Property *logData =
        ws->mutableRun().getLogData(twoThetaLogName);
    auto logPWV =
        dynamic_cast<const Kernel::PropertyWithValue<double> *>(logData);
    auto logTSP =
        dynamic_cast<const Kernel::TimeSeriesProperty<double> *>(logData);

    if (logPWV) {
      twoTheta = *logPWV;
    } else if (logTSP && logTSP->realSize() > 0) {
      twoTheta = logTSP->lastValue();
    } else {
      throw std::runtime_error(
          "Value for two theta could not be found in log.");
    }
    g_log.notice() << "Found '" << twoTheta
                   << "' as value for two theta in log." << std::endl;
  }

  Instrument_const_sptr instrument = ws->getInstrument();
  IComponent_const_sptr slit1 = instrument->getComponentByName(slit1Name);
  IComponent_const_sptr slit2 = instrument->getComponentByName(slit2Name);

  if (!slit1)
    throw std::runtime_error(
        "Could not find component in instrument with name: '" + slit1Name +
        "'");

  if (!slit2)
    throw std::runtime_error(
        "Could not find component in instrument with name: '" + slit2Name +
        "'");

  const V3D slitDiff =
      (slit2->getPos() - slit1->getPos()) * 1000; // Convert from mm to m.

  std::vector<double> slit1VGParam = slit1->getNumberParameter(vGapParam);
  std::vector<double> slit2VGParam = slit2->getNumberParameter(vGapParam);

  if (slit1VGParam.size() < 1)
    throw std::runtime_error("Could not find a value for the first slit's "
                             "vertical gap with given parameter name: '" +
                             vGapParam + "'.");

  if (slit2VGParam.size() < 1)
    throw std::runtime_error("Could not find a value for the second slit's "
                             "vertical gap with given parameter name: '" +
                             vGapParam + "'.");

  const double slit1VG = slit1VGParam[0];
  const double slit2VG = slit2VGParam[0];

  const double totalVertGap = slit1VG + slit2VG;
  const double slitDist =
      sqrt(slitDiff.X() * slitDiff.X() + slitDiff.Y() * slitDiff.Y() +
           slitDiff.Z() * slitDiff.Z());

  const double resolution =
      atan(totalVertGap / (2 * slitDist)) * 180.0 / M_PI / twoTheta;

  setProperty("Resolution", resolution);
  setProperty("TwoThetaOut", twoTheta);
}

} // namespace Algorithms
} // namespace Mantid
