#include "MantidDataHandling/SetSample.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/SampleEnvironmentFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/PropertyManagerProperty.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <Poco/Path.h>

namespace Mantid {
namespace DataHandling {

using Geometry::SampleEnvironment;
using Kernel::Logger;
using Kernel::V3D;

namespace {

/**
 * Retrieve "Axis" property value. The most commmon use case is calling
 * this algorithm from Python where Axis is input as a C long. The
 * definition of long varies across platforms and long v = args.getProperty()
 * is currently unable to cope so we go via the long route to retrieve
 * the value.
 * @param args Dictionary-type containing the argument
 */
long getAxisIndex(const Kernel::PropertyManager &args) {
  using Kernel::Property;
  using Kernel::PropertyWithValue;
  long axisIdx(1);
  if (args.existsProperty("Axis")) {
    Kernel::Property *axisProp = args.getProperty("Axis");
    axisIdx = static_cast<PropertyWithValue<long> *>(axisProp)->operator()();
    if (axisIdx < 0 || axisIdx > 2)
      throw std::invalid_argument(
          "Geometry.Axis value must be either 0,1,2 (X,Y,Z)");
  }
  return axisIdx;
}

/**
  * Return the centre coordinates of the base of a cylinder given the
  * coordinates of the centre of the cylinder
  * @param cylCentre Coordinates of centre of the cylinder (X,Y,Z) (in metres)
  * @param height Height of the cylinder (in metres)
  * @param axis The index of the height-axis of the cylinder
  */
V3D cylBaseCentre(const std::vector<double> &cylCentre, double height,
                  long axisIdx) {
  const V3D halfHeight = [&]() {
    switch (axisIdx) {
    case 0:
      return V3D(0.5 * height, 0, 0);
    case 1:
      return V3D(0, 0.5 * height, 0);
    case 2:
      return V3D(0, 0, 0.5 * height);
    default:
      return V3D();
    }
  }();
  return V3D(cylCentre[0], cylCentre[1], cylCentre[2]) - halfHeight;
}

/**
 * Create the xml tag require for a given axis index
 * @param axisIdx Index 0,1,2 for the axis of a cylinder
 * @return A string containing the axis tag for this index
 */
std::string axisXML(long axisIdx) {
  switch (axisIdx) {
  case 0:
    return "<axis x=\"1\" y=\"0\" z=\"0\" />";
  case 1:
    return "<axis x=\"0\" y=\"1\" z=\"0\" />";
  case 2:
    return "<axis x=\"0\" y=\"0\" z=\"1\" />";
  default:
    return "";
  }
}
}

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SetSample)

/// Algorithms name for identification. @see Algorithm::name
const std::string SetSample::name() const { return "SetSample"; }

/// Algorithm's version for identification. @see Algorithm::version
int SetSample::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SetSample::category() const { return "Sample"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string SetSample::summary() const {
  return "Set properties of the sample and its environment for a workspace";
}

/// Validate the inputs against each other @see Algorithm::validateInputs
std::map<std::string, std::string> SetSample::validateInputs() {
  using Kernel::PropertyManager_sptr;
  std::map<std::string, std::string> errors;

  // Validate Environment
  PropertyManager_sptr environArgs = getProperty("Environment");
  if (environArgs) {
    if (!environArgs->existsProperty("Name")) {
      errors["Environment"] = "Environment flags must contain a 'Name' entry.";
    } else {
      std::string name = environArgs->getPropertyValue("Name");
      if (name.empty()) {
        errors["Environment"] = "Environment 'Name' flag is an empty string!";
      }
    }

    if (!environArgs->existsProperty("Container")) {
      errors["Environment"] =
          "Environment flags must contain a 'Container' entry.";
    } else {
      std::string name = environArgs->getPropertyValue("Container");
      if (name.empty()) {
        errors["Environment"] = "Environment 'Can' flag is an empty string!";
      }
    }
  }

  return errors;
}

/**
 * Initialize the algorithm's properties.
 */
void SetSample::init() {
  using API::WorkspaceProperty;
  using Kernel::Direction;
  using Kernel::PropertyManagerProperty;

  // Inputs
  declareProperty(Kernel::make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                                           Direction::InOut),
                  "A workspace whose sample properties will be updated");
  declareProperty(Kernel::make_unique<PropertyManagerProperty>(
                      "Geometry", Direction::Input),
                  "A dictionary of geometry parameters for the sample.");
  declareProperty(Kernel::make_unique<PropertyManagerProperty>(
                      "Material", Direction::Input),
                  "A dictionary of material parameters for the sample. See "
                  "SetSampleMaterial for all accepted parameters");
  declareProperty(
      Kernel::make_unique<PropertyManagerProperty>("Environment",
                                                   Direction::Input),
      "A dictionary of parameters to configure the sample environment");
}

/**
 * Execute the algorithm.
 */
void SetSample::exec() {
  using API::MatrixWorkspace_sptr;
  using Kernel::PropertyManager_sptr;

  MatrixWorkspace_sptr workspace = getProperty("InputWorkspace");
  PropertyManager_sptr environArgs = getProperty("Environment");
  PropertyManager_sptr geometryArgs = getProperty("Geometry");
  PropertyManager_sptr materialArgs = getProperty("Material");

  // The order here is important. Se the environment first. If this
  // defines a sample geometry then we can process the Geometry flags
  // combined with this
  const SampleEnvironment *sampleEnviron(nullptr);
  if (environArgs) {
    sampleEnviron = setSampleEnvironment(workspace, *environArgs);
  }

  if (geometryArgs || sampleEnviron) {
    setSampleShape(workspace, geometryArgs, sampleEnviron);
  }

  // Finally the material arguments
  if (materialArgs) {
    runChildAlgorithm("SetSampleMaterial", workspace, *materialArgs);
  }
}

/**
 * Set the requested sample environment on the workspace
 * @param workspace A pointer to the workspace to be affected
 * @param args The dictionary of flags for the environment
 * @return A pointer to the new sample environment
 */
const Geometry::SampleEnvironment *
SetSample::setSampleEnvironment(API::MatrixWorkspace_sptr &workspace,
                                const Kernel::PropertyManager &args) {
  using Geometry::SampleEnvironmentSpecFileFinder;
  using Geometry::SampleEnvironmentFactory;
  using Kernel::ConfigService;

  const std::string envName = args.getPropertyValue("Name");
  const std::string canName = args.getPropertyValue("Container");
  // The specifications need to be qualified by the facility and instrument.
  // Check instrument for name and then lookup facility if facility
  // is unknown then set to default facility & instrument.
  auto instrument = workspace->getInstrument();
  const auto &instOnWS = instrument->getName();
  const auto &config = ConfigService::Instance();
  std::string facilityName, instrumentName;
  try {
    const auto &instInfo = config.getInstrument(instOnWS);
    instrumentName = instInfo.name();
    facilityName = instInfo.facility().name();
  } catch (std::runtime_error &) {
    // use default facility/instrument
    facilityName = config.getFacility().name();
    instrumentName = config.getInstrument().name();
  }

  const auto &instDirs = config.getInstrumentDirectories();
  std::vector<std::string> environDirs(instDirs);
  for (auto &direc : environDirs) {
    direc = Poco::Path(direc).append("sampleenvironments").toString();
  }
  auto finder =
      Kernel::make_unique<SampleEnvironmentSpecFileFinder>(environDirs);
  SampleEnvironmentFactory factory(std::move(finder));
  auto sampleEnviron =
      factory.create(facilityName, instrumentName, envName, canName);
  workspace->mutableSample().setEnvironment(sampleEnviron.release());

  return &(workspace->sample().getEnvironment());
}

/**
 * @param workspace A pointer to the workspace to be affected
 * @param args The user-supplied dictionary of flags
 * @param sampleEnv A pointer to the sample environment if one exists, otherwise
 * null
 * @return A string containing the XML definition of the shape
 */
void SetSample::setSampleShape(API::MatrixWorkspace_sptr &workspace,
                               const Kernel::PropertyManager_sptr &args,
                               const Geometry::SampleEnvironment *sampleEnv) {
  using Geometry::Container;
  /* The sample geometry can be specified in two ways:
     - a known set of primitive shapes with values or CSG string
     - or a <samplegeometry> field sample environment can, with values possible
       overridden by the Geometry flags
  */

  // Try known shapes or CSG first if supplied
  const auto xml = tryCreateXMLFromArgsOnly(args);
  if (!xml.empty()) {
    runSetSampleShape(workspace, xml);
    return;
  }
  // Any arguments in the args dict are assumed to be values that should
  // override the default set by the sampleEnv samplegeometry if it exists
  if (sampleEnv) {
    if (sampleEnv->container()->hasSampleShape()) {
      const auto &can = sampleEnv->container();
      Container::ShapeArgs shapeArgs;
      if (args) {
        const auto &props = args->getProperties();
        for (const auto &prop : props) {
          // assume in cm
          const double val = args->getProperty(prop->name());
          shapeArgs.emplace(boost::algorithm::to_lower_copy(prop->name()),
                            val * 0.01);
        }
      }
      auto shapeObject = can->createSampleShape(shapeArgs);
      // Set the object directly on the sample ensuring we preserve the
      // material
      const auto mat = workspace->sample().getMaterial();
      shapeObject->setMaterial(mat);
      workspace->mutableSample().setShape(*shapeObject);
    } else {
      throw std::runtime_error("The can does not define the sample shape. "
                               "Please either provide a 'Shape' argument "
                               "or update the environment definition with "
                               "this information.");
    }
  } else {
    throw std::runtime_error("No sample environment defined, please provide "
                             "a 'Shape' argument to define the sample "
                             "shape.");
  }
}

/**
 * Create the required XML for a given shape type plus its arguments
 * @param args A dict of flags defining the shape
 * @return A string containing the XML if possible or an empty string
 */
std::string
SetSample::tryCreateXMLFromArgsOnly(const Kernel::PropertyManager_sptr args) {
  std::string result;
  if (!args || !args->existsProperty("Shape")) {
    return result;
  }

  const auto shape = args->getPropertyValue("Shape");
  if (shape == "CSG") {
    result = args->getPropertyValue("Value");
  } else if (shape == "FlatPlate") {
    result = createFlatPlateXML(*args);
  } else if (boost::algorithm::ends_with(shape, "Cylinder")) {
    result = createCylinderLikeXML(
        *args, boost::algorithm::starts_with(shape, "Hollow"));
  } else {
    throw std::invalid_argument(
        "Unknown 'Shape' argument provided in "
        "'Geometry'. Allowed "
        "values=FlatPlate,CSG,Cylinder,HollowCylinder.");
  }
  if (g_log.is(Logger::Priority::PRIO_DEBUG)) {
    g_log.debug("XML shape definition:\n" + result + '\n');
  }
  return result;
}

/**
 * Create the XML required to define a flat plate from the given args
 * @param args A user-supplied dict of args
 * @return The XML definition string
 */
std::string
SetSample::createFlatPlateXML(const Kernel::PropertyManager &args) const {
  // X
  double widthInCM = args.getProperty("Width");
  // Y
  double heightInCM = args.getProperty("Height");
  // Z
  double thickInCM = args.getProperty("Thick");
  std::vector<double> center = args.getProperty("Center");
  // convert to metres
  std::transform(center.begin(), center.end(), center.begin(),
                 [](double val) { return val *= 0.01; });

  // Half lengths in metres (*0.01*0.5)
  const double szX = (widthInCM * 5e-3);
  const double szY = (heightInCM * 5e-3);
  const double szZ = (thickInCM * 5e-3);

  std::ostringstream xmlShapeStream;
  xmlShapeStream << " <cuboid id=\"sample-shape\"> "
                 << "<left-front-bottom-point x=\"" << szX + center[0]
                 << "\" y=\"" << -szY + center[1] << "\" z=\""
                 << -szZ + center[2] << "\"  /> "
                 << "<left-front-top-point  x=\"" << szX + center[0]
                 << "\" y=\"" << szY + center[1] << "\" z=\""
                 << -szZ + center[2] << "\"  /> "
                 << "<left-back-bottom-point  x=\"" << szX + center[0]
                 << "\" y=\"" << -szY + center[1] << "\" z=\""
                 << szZ + center[2] << "\"  /> "
                 << "<right-front-bottom-point  x=\"" << -szX + center[0]
                 << "\" y=\"" << -szY + center[1] << "\" z=\""
                 << -szZ + center[2] << "\"  /> "
                 << "</cuboid>";

  return xmlShapeStream.str();
}

/**
 * Create the XML required to define a cylinder from the given args
 * @param args A user-supplied dict of args
 * @param hollow True if an annulus is to be created
 * @return The XML definition string
 */
std::string
SetSample::createCylinderLikeXML(const Kernel::PropertyManager &args,
                                 bool hollow) const {
  const std::string tag = hollow ? "hollow-cylinder" : "cylinder";
  double height = args.getProperty("Height");
  double innerRadius = hollow ? args.getProperty("InnerRadius") : 0.0;
  double outerRadius =
      hollow ? args.getProperty("OuterRadius") : args.getProperty("Radius");
  std::vector<double> centre = args.getProperty("Center");
  long axisIdx = getAxisIndex(args);
  // convert to metres
  height *= 0.01;
  innerRadius *= 0.01;
  outerRadius *= 0.01;
  std::transform(centre.begin(), centre.end(), centre.begin(),
                 [](double val) { return val *= 0.01; });
  // XML needs center position of bottom base but user specifies center of
  // cylinder
  const V3D baseCentre = cylBaseCentre(centre, height, axisIdx);

  std::ostringstream xmlShapeStream;
  xmlShapeStream << "<" << tag << " id=\"sample-shape\"> "
                 << "<centre-of-bottom-base x=\"" << baseCentre.X() << "\" y=\""
                 << baseCentre.Y() << "\" z=\"" << baseCentre.Z() << "\" /> "
                 << axisXML(axisIdx) << "<height val=\"" << height << "\" /> ";
  if (hollow) {
    xmlShapeStream << "<inner-radius val=\"" << innerRadius << "\"/>"
                   << "<outer-radius val=\"" << outerRadius << "\"/>";

  } else {
    xmlShapeStream << "<radius val=\"" << outerRadius << "\"/>";
  }
  xmlShapeStream << "</" << tag << ">";
  return xmlShapeStream.str();
}

/**
 * Run SetSampleShape as an algorithm to set the shape of the sample
 * @param workspace A reference to the workspace
 * @param xml A string containing the XML definition
 */
void SetSample::runSetSampleShape(API::MatrixWorkspace_sptr &workspace,
                                  const std::string &xml) {
  auto alg = createChildAlgorithm("CreateSampleShape");
  alg->setProperty("InputWorkspace", workspace);
  alg->setProperty("ShapeXML", xml);
  alg->executeAsChildAlg();
}

/**
 * Run the named child algorithm on the given workspace. It assumes an in/out
 * workspace property called InputWorkspace
 * @param name The name of the algorithm to run
 * @param workspace A reference to the workspace
 * @param args A PropertyManager specifying the required arguments
 */
void SetSample::runChildAlgorithm(const std::string &name,
                                  API::MatrixWorkspace_sptr &workspace,
                                  const Kernel::PropertyManager &args) {
  auto alg = createChildAlgorithm(name);
  alg->setProperty("InputWorkspace", workspace);
  alg->updatePropertyValues(args);
  alg->executeAsChildAlg();
}

} // namespace DataHandling
} // namespace Mantid
