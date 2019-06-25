// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/NRCalculateSlitResolution.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
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
DECLARE_ALGORITHM(NRCalculateSlitResolution)

/// Algorithm's name for identification. @see Algorithm::name
const std::string NRCalculateSlitResolution::name() const {
  return "NRCalculateSlitResolution";
}

/// Algorithm's version for identification. @see Algorithm::version
int NRCalculateSlitResolution::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string NRCalculateSlitResolution::category() const {
  return "Reflectometry\\ISIS";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string NRCalculateSlitResolution::summary() const {
  return "Calculates the reflectometry resolution (dQ/Q) for a given "
         "workspace.";
}

/** Initialize the algorithm's properties.
 */
void NRCalculateSlitResolution::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>(
                      "Workspace", "", Direction::Input,
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
  declareProperty("ThetaLogName", "Theta",
                  "Name theta can be found in the run log as.");

  declareProperty("Resolution", Mantid::EMPTY_DBL(),
                  "Calculated resolution (dq/q).", Direction::Output);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void NRCalculateSlitResolution::exec() {
  const MatrixWorkspace_sptr ws = getProperty("Workspace");
  double twoTheta = getProperty("TwoTheta");
  const std::string slit1Name = getProperty("FirstSlitName");
  const std::string slit2Name = getProperty("SecondSlitName");
  const std::string vGapParam = getProperty("VerticalGapParameter");
  const std::string thetaLogName = getProperty("ThetaLogName");
  double theta = 0.0;

  if (!isEmpty(twoTheta)) {
    theta = twoTheta / 2.0;
  } else {
    const Kernel::Property *logData = ws->mutableRun().getLogData(thetaLogName);
    auto logPWV =
        dynamic_cast<const Kernel::PropertyWithValue<double> *>(logData);
    auto logTSP =
        dynamic_cast<const Kernel::TimeSeriesProperty<double> *>(logData);

    if (logPWV) {
      theta = *logPWV;
    } else if (logTSP && logTSP->realSize() > 0) {
      theta = logTSP->lastValue();
    } else {
      throw std::runtime_error(
          "Value for two theta could not be found in log.");
    }
    g_log.notice() << "Found '" << theta << "' as value for theta in log.\n";
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
      (slit2->getPos() - slit1->getPos()) * 1000; // Convert from m to mm.

  std::vector<double> slit1VGParam = slit1->getNumberParameter(vGapParam);
  std::vector<double> slit2VGParam = slit2->getNumberParameter(vGapParam);

  if (slit1VGParam.empty())
    throw std::runtime_error("Could not find a value for the first slit's "
                             "vertical gap with given parameter name: '" +
                             vGapParam + "'.");

  if (slit2VGParam.empty())
    throw std::runtime_error("Could not find a value for the second slit's "
                             "vertical gap with given parameter name: '" +
                             vGapParam + "'.");

  const double slit1VG = slit1VGParam[0];
  const double slit2VG = slit2VGParam[0];

  const double totalVertGap = slit1VG + slit2VG;
  const double slitDist =
      sqrt(slitDiff.X() * slitDiff.X() + slitDiff.Y() * slitDiff.Y() +
           slitDiff.Z() * slitDiff.Z());

  double resolution =
      atan(totalVertGap / slitDist) / (2 * std::tan(theta * M_PI / 180.0));

  setProperty("Resolution", resolution);
}

} // namespace Algorithms
} // namespace Mantid
