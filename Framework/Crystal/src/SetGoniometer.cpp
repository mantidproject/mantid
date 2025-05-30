// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/SetGoniometer.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MultipleExperimentInfos.h"
#include "MantidAPI/Run.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

using Mantid::Geometry::Goniometer;
using namespace Mantid::Geometry;

namespace Mantid::Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SetGoniometer)

using namespace Mantid::Kernel;
using namespace Mantid::API;

/// How many axes (max) to define
const size_t NUM_AXES = 6;

/** Initialize the algorithm's properties.
 */
void SetGoniometer::init() {

  auto threeVthree = std::make_shared<ArrayLengthValidator<double>>(9);
  std::vector<double> zeroes(9, 0.);

  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("Workspace", "", Direction::InOut),
                  "An workspace that will be modified with the new goniometer created.");

  std::vector<std::string> gonOptions{"None, Specify Individually", "Universal"};
  declareProperty("Goniometers", gonOptions[0], std::make_shared<StringListValidator>(gonOptions),
                  "Set the axes and motor names according to goniometers that "
                  "we define in the code (Universal defined for SNS)");

  std::string axisHelp = ": name, x,y,z, 1/-1 (1 for ccw, -1 for cw rotation). "
                         "A number of degrees can be used instead of name. "
                         "Leave blank for no axis";
  for (size_t i = 0; i < NUM_AXES; i++) {
    std::ostringstream propName;
    propName << "Axis" << i;
    declareProperty(std::make_unique<PropertyWithValue<std::string>>(propName.str(), "", Direction::Input),
                    propName.str() + axisHelp);
  }
  declareProperty("Average", true,
                  "Use the average value of the log, if false a separate "
                  "goniometer will be created for each value in the logs");

  declareProperty(std::make_unique<ArrayProperty<double>>("GoniometerMatrix", std::move(zeroes), threeVthree),
                  "Directly set the goniometer rotation matrix. Input should be in the form of a flattened 3x3 matrix");
  for (size_t i = 0; i < NUM_AXES; i++) {
    std::ostringstream propName;
    propName << "Axis" << i;
    setPropertySettings(propName.str(), std::make_unique<EnabledWhenProperty>("GoniometerMatrix", IS_DEFAULT));
  }
}

std::map<std::string, std::string> SetGoniometer::validateInputs() {
  std::vector<double> Gvec = getProperty("GoniometerMatrix");
  std::map<std::string, std::string> issues;
  const Kernel::DblMatrix GMatrix(Gvec);

  // if a Goniometer Matrix is supplied, check it is a valid rotation
  if (!isDefault("GoniometerMatrix")) {
    for (size_t i = 0; i < NUM_AXES; i++) {
      std::ostringstream propName;
      propName << "Axis" << i;
      if (!isDefault(propName.str())) {
        issues[propName.str()] = "Can't provide a goniometer axis if a matrix string has also been provided";
      }
    }

    if (GMatrix.numRows() == GMatrix.numCols()) {
      bool isRot = GMatrix.isRotation();
      if (!isRot) {
        issues["GoniometerMatrix"] = "Supplied Goniometer Matrix is not a proper rotation";
      }
    } else {
      // this should not be reached because of the input validator
      issues["GoniometerMatrix"] = "Supplied Goniometer Matrix is not a proper rotation: Matrix is not Square";
    }
  }
  return issues;
}

/** Execute the algorithm.
 */
void SetGoniometer::exec() {
  Workspace_sptr ws = getProperty("Workspace");
  auto ei = std::dynamic_pointer_cast<ExperimentInfo>(ws);

  if (!ei) {
    // We're dealing with an MD workspace which has multiple experiment infos
    auto infos = std::dynamic_pointer_cast<MultipleExperimentInfos>(ws);
    if (!infos) {
      throw std::invalid_argument("Input workspace does not support Goniometer");
    }
    if (infos->getNumExperimentInfo() < 1) {
      ExperimentInfo_sptr info(new ExperimentInfo());
      infos->addExperimentInfo(info);
    }
    ei = infos->getExperimentInfo(0);
  }

  // Create the goniometer
  Goniometer gon;

  if (isDefault("GoniometerMatrix")) {
    std::string gonioDefined = getPropertyValue("Goniometers");
    if (gonioDefined == "Universal")
      gon.makeUniversalGoniometer();
    else
      for (size_t i = 0; i < NUM_AXES; i++) {
        std::ostringstream propName;
        propName << "Axis" << i;
        std::string axisDesc = getPropertyValue(propName.str());

        if (!axisDesc.empty()) {
          std::vector<std::string> tokens;
          boost::split(tokens, axisDesc, boost::algorithm::detail::is_any_ofF<char>(","));
          if (tokens.size() != 5)
            throw std::invalid_argument("Wrong number of arguments to parameter " + propName.str() +
                                        ". Expected 5 comma-separated arguments.");

          std::string axisName = tokens[0];
          axisName = Strings::strip(axisName);
          if (axisName.empty())
            throw std::invalid_argument("The name must not be empty");

          // If axisName is a number, add a new log value
          double angle = 0;
          if (Strings::convert(axisName, angle)) {
            g_log.information() << "Axis " << i << " - create a new log value GoniometerAxis" << i << "_FixedValue\n";
            axisName = "GoniometerAxis" + Strings::toString(i) + "_FixedValue";
            try {
              Types::Core::DateAndTime now = Types::Core::DateAndTime::getCurrentTime();
              auto tsp = new Kernel::TimeSeriesProperty<double>(axisName);
              tsp->addValue(now, angle);
              tsp->setUnits("degree");
              if (ei->mutableRun().hasProperty(axisName)) {
                ei->mutableRun().removeLogData(axisName);
              }
              ei->mutableRun().addLogData(tsp);
            } catch (...) {
              g_log.error("Could not add axis");
            }
          }

          double x = 0, y = 0, z = 0;
          if (!Strings::convert(tokens[1], x))
            throw std::invalid_argument("Error converting string '" + tokens[1] + "' to a number.");
          if (!Strings::convert(tokens[2], y))
            throw std::invalid_argument("Error converting string '" + tokens[2] + "' to a number.");
          if (!Strings::convert(tokens[3], z))
            throw std::invalid_argument("Error converting string '" + tokens[3] + "' to a number.");
          V3D vec(x, y, z);
          if (vec.norm() < 1e-4)
            throw std::invalid_argument("Rotation axis vector should be non-zero!");

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
                         "identity rotation matrix.\n";

    // All went well, copy the goniometer into it. It will throw if the log values
    // cannot be found
    try {
      if (getProperty("Average"))
        ei->mutableRun().setGoniometer(gon, true);
      else
        ei->mutableRun().setGoniometers(gon);
    } catch (std::runtime_error &) {
      g_log.error("No log values for goniometers");
    }
  } else {
    const std::vector<double> gonVec = getProperty("GoniometerMatrix");
    const Kernel::DblMatrix gonMat(gonVec);
    ei->mutableRun().setGoniometer(Goniometer(gonMat), false);
  }
}

} // namespace Mantid::Crystal
