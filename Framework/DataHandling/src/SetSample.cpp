// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SetSample.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Instrument/SampleEnvironmentFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerProperty.h"

#include <Poco/Path.h>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>

namespace Mantid {
namespace DataHandling {

using Geometry::Goniometer;
using Geometry::ReferenceFrame;
using Geometry::SampleEnvironment;
using Kernel::Logger;
using Kernel::PropertyWithValue;
using Kernel::V3D;

namespace {
constexpr double CUBIC_METRE_TO_CM = 100. * 100. * 100.;

/// Private namespace storing property name strings
namespace PropertyNames {
/// Input workspace property name
const std::string INPUT_WORKSPACE("InputWorkspace");
/// Geometry property name
const std::string GEOMETRY("Geometry");
/// Material property name
const std::string MATERIAL("Material");
/// Environment property name
const std::string ENVIRONMENT("Environment");
} // namespace PropertyNames
/// Private namespace storing sample environment args
namespace SEArgs {
/// Static Name string
const std::string NAME("Name");
/// Static Container string
const std::string CONTAINER("Container");
} // namespace SEArgs
/// Provate namespace storing geometry args
namespace GeometryArgs {
/// Static Shape string
const std::string SHAPE("Shape");
} // namespace GeometryArgs

/// Private namespace storing sample environment args
namespace ShapeArgs {
/// Static FlatPlate string
const std::string FLAT_PLATE("FlatPlate");
/// Static Cylinder string
const std::string CYLINDER("Cylinder");
/// Static HollowCylinder string
const std::string HOLLOW_CYLINDER("HollowCylinder");
/// Static CSG string
const std::string CSG("CSG");
/// Static Width string
const std::string WIDTH("Width");
/// Static Height string
const std::string HEIGHT("Height");
/// Static Thick string
const std::string THICK("Thick");
/// Static Axis string
const std::string AXIS("Axis");
/// Static Center string
const std::string CENTER("Center");
/// Static Radius string
const std::string RADIUS("Radius");
/// Static InnerRadius string
const std::string INNER_RADIUS("InnerRadius");
/// Static OuterRadius string
const std::string OUTER_RADIUS("OuterRadius");
} // namespace ShapeArgs

/**
 * Return the centre coordinates of the base of a cylinder given the
 * coordinates of the centre of the cylinder
 * @param cylCentre Coordinates of centre of the cylinder (X,Y,Z) (in metres)
 * @param height Height of the cylinder (in metres)
 * @param axis The index of the height-axis of the cylinder
 */
V3D cylBaseCentre(const std::vector<double> &cylCentre, double height,
                  unsigned axisIdx) {
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
 * Return the centre coordinates of the base of a cylinder given the
 * coordinates of the centre of the cylinder
 * @param cylCentre Coordinates of centre of the cylinder (X,Y,Z) (in metres)
 * @param height Height of the cylinder (in metres)
 * @param axis The height-axis of the cylinder
 */
V3D cylBaseCentre(const std::vector<double> &cylCentre, double height,
                  std::vector<double> axis) {
  using Kernel::V3D;
  V3D axisVector = V3D{axis[0], axis[1], axis[2]};
  axisVector.normalize();
  return V3D(cylCentre[0], cylCentre[1], cylCentre[2]) -
         axisVector * height * 0.5;
}

/**
 * Create the xml tag require for a given axis index
 * @param axisIdx Index 0,1,2 for the axis of a cylinder
 * @return A string containing the axis tag for this index
 */
std::string axisXML(unsigned axisIdx) {
  switch (axisIdx) {
  case 0:
    return R"(<axis x="1" y="0" z="0" />)";
  case 1:
    return R"(<axis x="0" y="1" z="0" />)";
  case 2:
    return R"(<axis x="0" y="0" z="1" />)";
  default:
    return "";
  }
}

/**
 * Create the xml tag require for a given axis
 * @param axis 3D vector of double
 * @return A string containing the axis tag representation
 */
std::string axisXML(std::vector<double> axis) {
  std::ostringstream str;
  str << "<axis x=\"" << axis[0] << "\" y=\"" << axis[1] << "\" z=\"" << axis[2]
      << "\" /> ";
  return str.str();
}

/**
 * Return a property as type double if possible. Checks for either a
 * double or an int property and casts accordingly
 * @param args A reference to the property manager
 * @param name The name of the property
 * @return The value of the property as a double
 * @throws Exception::NotFoundError if the property does not exist
 */
double getPropertyAsDouble(const Kernel::PropertyManager &args,
                           const std::string &name) {
  try {
    return args.getProperty(name);
  } catch (std::runtime_error &) {
    return static_cast<int>(args.getProperty(name));
  }
}

/**
 * Return a property as type vector<double> if possible. Checks for either a
 * vector<double> or a vector<int> property and casts accordingly
 * @param args A reference to the property manager
 * @param name The name of the property
 * @return The value of the property as a double
 * @throws Exception::NotFoundError if the property does not exist
 */
std::vector<double>
getPropertyAsVectorDouble(const Kernel::PropertyManager &args,
                          const std::string &name) {
  try {
    return args.getProperty(name);
  } catch (std::runtime_error &) {
    std::vector<int> intValues = args.getProperty(name);
    std::vector<double> dblValues(std::begin(intValues), std::end(intValues));
    return dblValues;
  }
}

} // namespace

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
  using Kernel::PropertyManager;
  using Kernel::PropertyManager_const_sptr;
  std::map<std::string, std::string> errors;

  auto existsAndNotEmptyString = [](const PropertyManager &pm,
                                    const std::string &name) {
    if (pm.existsProperty(name)) {
      const auto value = pm.getPropertyValue(name);
      return !value.empty();
    }
    return false;
  };

  auto existsAndNegative = [](const PropertyManager &pm,
                              const std::string &name) {
    if (pm.existsProperty(name)) {
      const auto value = pm.getPropertyValue(name);
      if (boost::lexical_cast<double>(value) < 0.0) {
        return true;
      }
    }
    return false;
  };

  // Validate Environment
  const PropertyManager_const_sptr environArgs =
      getProperty(PropertyNames::ENVIRONMENT);
  if (environArgs) {
    if (!existsAndNotEmptyString(*environArgs, SEArgs::NAME)) {
      errors[PropertyNames::ENVIRONMENT] =
          "Environment flags require a non-empty 'Name' entry.";
    }

    if (!existsAndNotEmptyString(*environArgs, SEArgs::CONTAINER)) {
      errors[PropertyNames::ENVIRONMENT] =
          "Environment flags require a non-empty 'Container' entry.";
    }
  }

  // Validate as much of the shape information as possible
  const PropertyManager_const_sptr geomArgs =
      getProperty(PropertyNames::GEOMETRY);
  if (geomArgs) {
    if (existsAndNotEmptyString(*geomArgs, GeometryArgs::SHAPE)) {
      const std::array<const std::string *, 6> positiveValues = {
          {&ShapeArgs::HEIGHT, &ShapeArgs::WIDTH, &ShapeArgs::THICK,
           &ShapeArgs::RADIUS, &ShapeArgs::INNER_RADIUS,
           &ShapeArgs::OUTER_RADIUS}};
      for (const auto &arg : positiveValues) {
        if (existsAndNegative(*geomArgs, *arg)) {
          errors[PropertyNames::GEOMETRY] = *arg + " argument < 0.0";
        }
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
  declareProperty(Kernel::make_unique<WorkspaceProperty<>>(
                      PropertyNames::INPUT_WORKSPACE, "", Direction::InOut),
                  "A workspace whose sample properties will be updated");
  declareProperty(Kernel::make_unique<PropertyManagerProperty>(
                      PropertyNames::GEOMETRY, Direction::Input),
                  "A dictionary of geometry parameters for the sample.");
  declareProperty(Kernel::make_unique<PropertyManagerProperty>(
                      PropertyNames::MATERIAL, Direction::Input),
                  "A dictionary of material parameters for the sample. See "
                  "SetSampleMaterial for all accepted parameters");
  declareProperty(
      Kernel::make_unique<PropertyManagerProperty>(PropertyNames::ENVIRONMENT,
                                                   Direction::Input),
      "A dictionary of parameters to configure the sample environment");
}

/**
 * Execute the algorithm.
 */
void SetSample::exec() {
  using API::MatrixWorkspace_sptr;
  using Kernel::PropertyManager_sptr;

  MatrixWorkspace_sptr workspace = getProperty(PropertyNames::INPUT_WORKSPACE);
  PropertyManager_sptr environArgs = getProperty(PropertyNames::ENVIRONMENT);
  PropertyManager_sptr geometryArgs = getProperty(PropertyNames::GEOMETRY);
  PropertyManager_sptr materialArgs = getProperty(PropertyNames::MATERIAL);

  // The order here is important. Se the environment first. If this
  // defines a sample geometry then we can process the Geometry flags
  // combined with this
  const SampleEnvironment *sampleEnviron(nullptr);
  if (environArgs) {
    sampleEnviron = setSampleEnvironment(workspace, environArgs);
  }

  double sampleVolume = 0.;
  if (geometryArgs || sampleEnviron) {
    setSampleShape(workspace, geometryArgs, sampleEnviron);
    // get the volume back out to use in setting the material
    sampleVolume = CUBIC_METRE_TO_CM * workspace->sample().getShape().volume();
  }

  // Finally the material arguments
  if (materialArgs) {
    // add the sample volume if it was defined/determined
    if (sampleVolume > 0.) {
      const std::string VOLUME_ARG{"SampleVolume"};
      // only add the volume if it isn't already specfied
      if (!materialArgs->existsProperty(VOLUME_ARG)) {
        materialArgs->declareProperty(
            Kernel::make_unique<PropertyWithValue<double>>(VOLUME_ARG,
                                                           sampleVolume));
      }
    }
    runChildAlgorithm("SetSampleMaterial", workspace, *materialArgs);
  }
}

/**
 * Set the requested sample environment on the workspace
 * @param workspace A pointer to the workspace to be affected
 * @param args The dictionary of flags for the environment
 * @return A pointer to the new sample environment
 */
const Geometry::SampleEnvironment *SetSample::setSampleEnvironment(
    API::MatrixWorkspace_sptr &workspace,
    const Kernel::PropertyManager_const_sptr &args) {
  using Geometry::SampleEnvironmentFactory;
  using Geometry::SampleEnvironmentSpecFileFinder;
  using Kernel::ConfigService;

  const std::string envName = args->getPropertyValue(SEArgs::NAME);
  const std::string canName = args->getPropertyValue(SEArgs::CONTAINER);
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
  workspace->mutableSample().setEnvironment(std::move(sampleEnviron));
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
                               const Kernel::PropertyManager_const_sptr &args,
                               const Geometry::SampleEnvironment *sampleEnv) {
  using Geometry::Container;
  /* The sample geometry can be specified in two ways:
     - a known set of primitive shapes with values or CSG string
     - or a <samplegeometry> field sample environment can, with values possible
       overridden by the Geometry flags
  */

  // Try known shapes or CSG first if supplied
  if (args) {
    const auto refFrame = workspace->getInstrument()->getReferenceFrame();
    const auto xml = tryCreateXMLFromArgsOnly(*args, *refFrame);
    if (!xml.empty()) {
      runSetSampleShape(workspace, xml);
      return;
    }
  }
  // Any arguments in the args dict are assumed to be values that should
  // override the default set by the sampleEnv samplegeometry if it exists
  if (sampleEnv) {
    if (sampleEnv->getContainer().hasSampleShape()) {
      const auto &can = sampleEnv->getContainer();
      Container::ShapeArgs shapeArgs;
      if (args) {
        const auto &props = args->getProperties();
        for (const auto &prop : props) {
          // assume in cm
          const double val = getPropertyAsDouble(*args, prop->name());
          shapeArgs.emplace(boost::algorithm::to_lower_copy(prop->name()),
                            val * 0.01);
        }
      }
      auto shapeObject = can.createSampleShape(shapeArgs);
      // Given that the object is a CSG object, set the object
      // directly on the sample ensuring we preserve the
      // material.
      const auto mat = workspace->sample().getMaterial();
      if (auto csgObj =
              boost::dynamic_pointer_cast<Geometry::CSGObject>(shapeObject)) {
        csgObj->setMaterial(mat);
      }
      workspace->mutableSample().setShape(shapeObject);
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
 * @param refFrame Defines the reference frame for the shape
 * @return A string containing the XML if possible or an empty string
 */
std::string
SetSample::tryCreateXMLFromArgsOnly(const Kernel::PropertyManager &args,
                                    const Geometry::ReferenceFrame &refFrame) {
  std::string result;
  if (!args.existsProperty(GeometryArgs::SHAPE)) {
    return result;
  }

  const auto shape = args.getPropertyValue(GeometryArgs::SHAPE);
  if (shape == ShapeArgs::CSG) {
    result = args.getPropertyValue("Value");
  } else if (shape == ShapeArgs::FLAT_PLATE) {
    result = createFlatPlateXML(args, refFrame);
  } else if (boost::algorithm::ends_with(shape, ShapeArgs::CYLINDER)) {
    result = createCylinderLikeXML(
        args, refFrame,
        boost::algorithm::equals(shape, ShapeArgs::HOLLOW_CYLINDER));
  } else {
    std::stringstream msg;
    msg << "Unknown 'Shape' argument '" << shape
        << "' provided in 'Geometry' property. Allowed values are "
        << ShapeArgs::CSG << ", " << ShapeArgs::FLAT_PLATE << ", "
        << ShapeArgs::CYLINDER << ", " << ShapeArgs::HOLLOW_CYLINDER;
    throw std::invalid_argument(msg.str());
  }
  if (g_log.is(Logger::Priority::PRIO_DEBUG)) {
    g_log.debug("XML shape definition:\n" + result + '\n');
  }
  return result;
}

/**
 * Create the XML required to define a flat plate from the given args
 * @param args A user-supplied dict of args
 * @param refFrame Defines the reference frame for the shape
 * @return The XML definition string
 */
std::string
SetSample::createFlatPlateXML(const Kernel::PropertyManager &args,
                              const Geometry::ReferenceFrame &refFrame) const {
  // Helper to take 3 coordinates and turn them to a V3D respecting the
  // current reference frame
  auto makeV3D = [&refFrame](double x, double y, double z) {
    V3D v;
    v[refFrame.pointingHorizontal()] = x;
    v[refFrame.pointingUp()] = y;
    v[refFrame.pointingAlongBeam()] = z;
    return v;
  };
  const double widthInCM = getPropertyAsDouble(args, ShapeArgs::WIDTH);
  const double heightInCM = getPropertyAsDouble(args, ShapeArgs::HEIGHT);
  const double thickInCM = getPropertyAsDouble(args, ShapeArgs::THICK);
  // Convert to half-"width" in metres
  const double szX = (widthInCM * 5e-3);
  const double szY = (heightInCM * 5e-3);
  const double szZ = (thickInCM * 5e-3);
  // Contruct cuboid corners. Define points about origin, rotate and then
  // translate to final center position
  auto lfb = makeV3D(szX, -szY, -szZ);
  auto lft = makeV3D(szX, szY, -szZ);
  auto lbb = makeV3D(szX, -szY, szZ);
  auto rfb = makeV3D(-szX, -szY, -szZ);
  // optional rotation about the center of object
  if (args.existsProperty("Angle")) {
    Goniometer gr;
    const auto upAxis = makeV3D(0, 1, 0);
    gr.pushAxis("up", upAxis.X(), upAxis.Y(), upAxis.Z(),
                args.getProperty("Angle"), Geometry::CCW, Geometry::angDegrees);
    auto &rotation = gr.getR();
    lfb.rotate(rotation);
    lft.rotate(rotation);
    lbb.rotate(rotation);
    rfb.rotate(rotation);
  }
  std::vector<double> center = args.getProperty(ShapeArgs::CENTER);
  const V3D centrePos(center[0] * 0.01, center[1] * 0.01, center[2] * 0.01);
  // translate to true center after rotation
  lfb += centrePos;
  lft += centrePos;
  lbb += centrePos;
  rfb += centrePos;

  std::ostringstream xmlShapeStream;
  xmlShapeStream << " <cuboid id=\"sample-shape\"> "
                 << "<left-front-bottom-point x=\"" << lfb.X() << "\" y=\""
                 << lfb.Y() << "\" z=\"" << lfb.Z() << "\"  /> "
                 << "<left-front-top-point  x=\"" << lft.X() << "\" y=\""
                 << lft.Y() << "\" z=\"" << lft.Z() << "\"  /> "
                 << "<left-back-bottom-point  x=\"" << lbb.X() << "\" y=\""
                 << lbb.Y() << "\" z=\"" << lbb.Z() << "\"  /> "
                 << "<right-front-bottom-point  x=\"" << rfb.X() << "\" y =\""
                 << rfb.Y() << "\" z=\"" << rfb.Z() << "\"  /> "
                 << "</cuboid>";
  return xmlShapeStream.str();
}

/**
 * Create the XML required to define a cylinder from the given args
 * @param args A user-supplied dict of args
 * @param refFrame Defines the reference frame for the shape
 * @param hollow True if an annulus is to be created
 * @return The XML definition string
 */
std::string
SetSample::createCylinderLikeXML(const Kernel::PropertyManager &args,
                                 const Geometry::ReferenceFrame &refFrame,
                                 bool hollow) const {
  const std::string tag = hollow ? "hollow-cylinder" : "cylinder";
  double height = getPropertyAsDouble(args, ShapeArgs::HEIGHT);
  double innerRadius =
      hollow ? getPropertyAsDouble(args, ShapeArgs::INNER_RADIUS) : 0.0;
  double outerRadius = hollow
                           ? getPropertyAsDouble(args, ShapeArgs::OUTER_RADIUS)
                           : getPropertyAsDouble(args, "Radius");
  std::vector<double> centre =
      getPropertyAsVectorDouble(args, ShapeArgs::CENTER);
  // convert to metres
  height *= 0.01;
  innerRadius *= 0.01;
  outerRadius *= 0.01;
  std::transform(centre.begin(), centre.end(), centre.begin(),
                 [](double val) { return val *= 0.01; });
  // XML needs center position of bottom base but user specifies center of
  // cylinder
  V3D baseCentre;
  std::ostringstream XMLString;
  if (args.existsProperty(ShapeArgs::AXIS)) {
    try {
      const int axisInt = args.getProperty(ShapeArgs::AXIS);
      const unsigned axisId = static_cast<unsigned>(axisInt);
      XMLString << axisXML(axisId);
      baseCentre = cylBaseCentre(centre, height, axisId);
    } catch (std::runtime_error &) {
      const std::vector<double> axis =
          getPropertyAsVectorDouble(args, ShapeArgs::AXIS);
      XMLString << axisXML(axis);
      baseCentre = cylBaseCentre(centre, height, axis);
    }
  } else {
    const unsigned axisId = static_cast<unsigned>(refFrame.pointingUp());
    XMLString << axisXML(axisId);
    baseCentre = cylBaseCentre(centre, height, axisId);
  }

  std::ostringstream xmlShapeStream;
  xmlShapeStream << "<" << tag << " id=\"sample-shape\"> "
                 << "<centre-of-bottom-base x=\"" << baseCentre.X() << "\" y=\""
                 << baseCentre.Y() << "\" z=\"" << baseCentre.Z() << "\" /> "
                 << XMLString.str() << "<height val=\"" << height << "\" /> ";
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
  alg->setProperty(PropertyNames::INPUT_WORKSPACE, workspace);
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
  alg->setProperty(PropertyNames::INPUT_WORKSPACE, workspace);
  alg->updatePropertyValues(args);
  alg->executeAsChildAlg();
}

} // namespace DataHandling
} // namespace Mantid
