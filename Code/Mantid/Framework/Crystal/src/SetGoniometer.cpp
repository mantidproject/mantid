#include "MantidCrystal/SetGoniometer.h"
#include "MantidKernel/System.h"
#include <boost/algorithm/string/split.hpp>
#include "MantidKernel/Strings.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include <boost/algorithm/string/detail/classification.hpp>
#include "MantidKernel/V3D.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/TimeSeriesProperty.h"

using Mantid::Geometry::Goniometer;
using namespace Mantid::Geometry;

namespace Mantid {
namespace Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SetGoniometer)

using namespace Mantid::Kernel;
using namespace Mantid::API;

/// How many axes (max) to define
const size_t NUM_AXES = 6;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SetGoniometer::SetGoniometer() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SetGoniometer::~SetGoniometer() {}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SetGoniometer::init() {
  declareProperty(
      new WorkspaceProperty<>("Workspace", "", Direction::InOut),
      "An workspace that will be modified with the new goniometer created.");

  std::vector<std::string> gonOptions;
  gonOptions.push_back("None, Specify Individually");
  gonOptions.push_back("Universal");
  declareProperty("Goniometers", gonOptions[0],
                  boost::make_shared<StringListValidator>(gonOptions),
                  "Set the axes and motor names according to goniometers that "
                  "we define in the code (Universal defined for SNS)");

  std::string axisHelp = ": name, x,y,z, 1/-1 (1 for ccw, -1 for cw rotation). "
                         "A number of degrees can be used instead of name. "
                         "Leave blank for no axis";
  for (size_t i = 0; i < NUM_AXES; i++) {
    std::ostringstream propName;
    propName << "Axis" << i;
    declareProperty(new PropertyWithValue<std::string>(propName.str(), "",
                                                       Direction::Input),
                    propName.str() + axisHelp);
  }
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SetGoniometer::exec() {
  MatrixWorkspace_sptr ws = getProperty("Workspace");
  std::string gonioDefined = getPropertyValue("Goniometers");
  // Create the goniometer
  Goniometer gon;

  if (gonioDefined.compare("Universal") == 0)
    gon.makeUniversalGoniometer();
  else
    for (size_t i = 0; i < NUM_AXES; i++) {
      std::ostringstream propName;
      propName << "Axis" << i;
      std::string axisDesc = getPropertyValue(propName.str());

      if (!axisDesc.empty()) {
        std::vector<std::string> tokens;
        boost::split(tokens, axisDesc,
                     boost::algorithm::detail::is_any_ofF<char>(","));
        if (tokens.size() != 5)
          throw std::invalid_argument(
              "Wrong number of arguments to parameter " + propName.str() +
              ". Expected 5 comma-separated arguments.");

        std::string axisName = tokens[0];
        axisName = Strings::strip(axisName);
        if (axisName.empty())
          throw std::invalid_argument("The name must not be empty");

        // If axisName is a number, add a new log value
        double angle = 0;
        if (Strings::convert(axisName, angle)) {
          g_log.information() << "Axis " << i
                              << " - create a new log value GoniometerAxis" << i
                              << "_FixedValue" << std::endl;
          axisName = "GoniometerAxis" + Strings::toString(i) + "_FixedValue";
          try {
            Kernel::DateAndTime now = Kernel::DateAndTime::getCurrentTime();
            Kernel::TimeSeriesProperty<double> *tsp =
                new Kernel::TimeSeriesProperty<double>(axisName);
            tsp->addValue(now, angle);
            tsp->setUnits("degree");
            if (ws->mutableRun().hasProperty(axisName)) {
              ws->mutableRun().removeLogData(axisName);
            }
            ws->mutableRun().addLogData(tsp);
          } catch (...) {
            g_log.error("Could not add axis");
          }
        }

        double x = 0, y = 0, z = 0;
        if (!Strings::convert(tokens[1], x))
          throw std::invalid_argument("Error converting string '" + tokens[1] +
                                      "' to a number.");
        if (!Strings::convert(tokens[2], y))
          throw std::invalid_argument("Error converting string '" + tokens[2] +
                                      "' to a number.");
        if (!Strings::convert(tokens[3], z))
          throw std::invalid_argument("Error converting string '" + tokens[3] +
                                      "' to a number.");
        V3D vec(x, y, z);
        if (vec.norm() < 1e-4)
          throw std::invalid_argument(
              "Rotation axis vector should be non-zero!");

        int ccw = 0;
        Strings::convert(tokens[4], ccw);
        if (ccw != 1 && ccw != -1)
          throw std::invalid_argument("The ccw parameter must be 1 (ccw) or -1 "
                                      "(cw) but no other value.");
        // Default to degrees
        gon.pushAxis(axisName, x, y, z, 0.0, ccw);
      }
    }

  if (gon.getNumberAxes() == 0)
    g_log.warning() << "Empty goniometer created; will always return an "
                       "identity rotation matrix." << std::endl;

  // All went well, copy the goniometer into it. It will throw if the log values
  // cannot be found
  try {
    ws->mutableRun().setGoniometer(gon, true);
  } catch (std::runtime_error &) {
    g_log.error("No log values for goniometers");
  }
}

} // namespace Mantid
} // namespace Crystal
