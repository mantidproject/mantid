// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SetSample.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/CreateSampleShape.h"
#include "MantidDataHandling/SampleEnvironmentFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Container.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/MaterialBuilder.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerProperty.h"

#include <Poco/Path.h>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>

namespace Mantid::DataHandling {

using API::ExperimentInfo;
using API::Workspace_sptr;
using Geometry::Container;
using Geometry::Goniometer;
using Geometry::ReferenceFrame;
using Geometry::SampleEnvironment;
using Geometry::ShapeFactory;
using Kernel::Logger;
using Kernel::MaterialBuilder;
using Kernel::PropertyManager;
using Kernel::PropertyManager_const_sptr;
using Kernel::PropertyWithValue;
using Kernel::V3D;

namespace {
constexpr double CUBIC_METRE_TO_CM = 100. * 100. * 100.;
constexpr double degToRad(const double x) { return x * M_PI / 180.; }

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
/// Container geometry property name
const std::string CONTAINER_GEOMETRY("ContainerGeometry");
/// Container material property name
const std::string CONTAINER_MATERIAL("ContainerMaterial");
} // namespace PropertyNames
/// Private namespace storing sample environment args
namespace SEArgs {
/// Static Name string
const std::string NAME("Name");
/// Static Container string
const std::string CONTAINER("Container");
/// Static Path string
const std::string PATH("Path");
} // namespace SEArgs
/// Provate namespace storing geometry args
namespace GeometryArgs {
/// Static Shape string
const std::string SHAPE("Shape");
/// Static Value string for CSG
const std::string VALUE("Value");
} // namespace GeometryArgs

/// Private namespace storing sample environment args
namespace ShapeArgs {
/// Static FlatPlate string
const std::string FLAT_PLATE("FlatPlate");
/// Static Cylinder string
const std::string CYLINDER("Cylinder");
/// Static HollowCylinder string
const std::string HOLLOW_CYLINDER("HollowCylinder");
/// Static Sphere string
const std::string SPHERE("Sphere");
/// Static FlatPlateHolder string
const std::string FLAT_PLATE_HOLDER("FlatPlateHolder");
/// Static HollowCylinderHolder string
const std::string HOLLOW_CYLINDER_HOLDER("HollowCylinderHolder");
/// Static CSG string
const std::string CSG("CSG");
/// Static Width string
const std::string WIDTH("Width");
/// Static Height string
const std::string HEIGHT("Height");
/// Static Thick string
const std::string THICK("Thick");
/// Static FrontThick string
const std::string FRONT_THICK("FrontThick");
/// Static BackThick string
const std::string BACK_THICK("BackThick");
/// Static Axis string
const std::string AXIS("Axis");
/// Static Angle string
const std::string ANGLE("Angle");
/// Static Center string
const std::string CENTER("Center");
/// Static Radius string
const std::string RADIUS("Radius");
/// Static InnerRadius string
const std::string INNER_RADIUS("InnerRadius");
/// Static OuterRadius string
const std::string OUTER_RADIUS("OuterRadius");
/// Static InnerOuterRadius string
const std::string INNER_OUTER_RADIUS("InnerOuterRadius");
/// Static OuterInnerRadius string
const std::string OUTER_INNER_RADIUS("OuterInnerRadius");
} // namespace ShapeArgs

/**
 * Return the centre coordinates of the base of a cylinder given the
 * coordinates of the centre of the cylinder
 * @param cylCentre Coordinates of centre of the cylinder (X,Y,Z) (in metres)
 * @param height Height of the cylinder (in metres)
 * @param axis The index of the height-axis of the cylinder
 */
V3D cylBaseCentre(const std::vector<double> &cylCentre, double height, unsigned axisIdx) {
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
V3D cylBaseCentre(const std::vector<double> &cylCentre, double height, const std::vector<double> &axis) {
  using Kernel::V3D;
  V3D axisVector = V3D{axis[0], axis[1], axis[2]};
  axisVector.normalize();
  return V3D(cylCentre[0], cylCentre[1], cylCentre[2]) - axisVector * height * 0.5;
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
std::string axisXML(const std::vector<double> &axis) {
  std::ostringstream str;
  str << "<axis x=\"" << axis[0] << "\" y=\"" << axis[1] << "\" z=\"" << axis[2] << "\" /> ";
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
double getPropertyAsDouble(const Kernel::PropertyManager &args, const std::string &name) {
  return std::stod(args.getPropertyValue(name));
}

/**
 * Return a property as type vector<double> if possible. Checks for either a
 * vector<double> or a vector<int> property and casts accordingly
 * @param args A reference to the property manager
 * @param name The name of the property
 * @return The value of the property as a vector<double>
 * @throws Exception::NotFoundError if the property does not exist
 */
std::vector<double> getPropertyAsVectorDouble(const Kernel::PropertyManager &args, const std::string &name) {
  std::string vectorAsString = args.getPropertyValue(name);
  std::vector<double> vectorOfDoubles;
  std::stringstream ss(vectorAsString);
  std::string elementAsString;
  while (std::getline(ss, elementAsString, ',')) {
    vectorOfDoubles.push_back(std::stod(elementAsString));
  }
  return vectorOfDoubles;
}

/**
 * @brief Returns if a property exists and is not empty
 * @param pm PropertyManager
 * @param name the name of the property
 * @return true if property with name exists, and its value is not empty
 */
bool existsAndNotEmptyString(const PropertyManager &pm, const std::string &name) {
  if (pm.existsProperty(name)) {
    const auto value = pm.getPropertyValue(name);
    return !value.empty();
  }
  return false;
}

/**
 * @brief Returns if a property exists and the numeric value is negative
 * @param pm PropertyManager
 * @param name the name of the property
 * @return true if property with name exists, but the numeric value is negative
 */
bool existsAndNegative(const PropertyManager &pm, const std::string &name) {
  if (pm.existsProperty(name)) {
    const auto value = pm.getPropertyValue(name);
    if (boost::lexical_cast<double>(value) < 0.0) {
      return true;
    }
  }
  return false;
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

/**
 * @brief Validates the geometry
 * @param errors map
 * @param geomArgs geometry arguments
 * @param flavour sample or container
 */
void SetSample::validateGeometry(std::map<std::string, std::string> &errors, const Kernel::PropertyManager &geomArgs,
                                 const std::string &flavour) {

  // Validate as much of the shape information as possible
  if (existsAndNotEmptyString(geomArgs, GeometryArgs::SHAPE)) {
    auto shape = geomArgs.getPropertyValue(GeometryArgs::SHAPE);
    if (shape == ShapeArgs::CSG) {
      if (!existsAndNotEmptyString(geomArgs, GeometryArgs::VALUE)) {
        errors[flavour] = "For " + shape + " shape " + GeometryArgs::VALUE + " is required";
      } else {
        // check if the value is a valid shape XML
        ShapeFactory shapeFactory;
        auto shapeFromValue = shapeFactory.createShape(geomArgs.getPropertyValue(GeometryArgs::VALUE));
        if (!shapeFromValue || !shapeFromValue->hasValidShape()) {
          errors[flavour] = "Invalid XML for CSG shape value";
        }
      }
    } else {
      if (shape == ShapeArgs::FLAT_PLATE || shape == ShapeArgs::FLAT_PLATE_HOLDER) {
        if (!existsAndNotEmptyString(geomArgs, ShapeArgs::WIDTH)) {
          errors[flavour] = "For " + shape + " shape " + ShapeArgs::WIDTH + " is required";
        }
        if (!existsAndNotEmptyString(geomArgs, ShapeArgs::THICK)) {
          errors[flavour] = "For " + shape + " shape " + ShapeArgs::THICK + " is required";
        }
        if (!existsAndNotEmptyString(geomArgs, ShapeArgs::HEIGHT)) {
          errors[flavour] = "For " + shape + " shape " + ShapeArgs::HEIGHT + " is required";
        }
      }
      if (shape == ShapeArgs::CYLINDER) {
        if (!existsAndNotEmptyString(geomArgs, ShapeArgs::RADIUS)) {
          errors[flavour] = "For " + shape + " shape " + ShapeArgs::RADIUS + " is required";
        }
        if (!existsAndNotEmptyString(geomArgs, ShapeArgs::HEIGHT)) {
          errors[flavour] = "For " + shape + " shape " + ShapeArgs::HEIGHT + " is required";
        }
      }
      if (shape == ShapeArgs::HOLLOW_CYLINDER || shape == ShapeArgs::HOLLOW_CYLINDER_HOLDER) {
        if (!existsAndNotEmptyString(geomArgs, ShapeArgs::INNER_RADIUS)) {
          errors[flavour] = "For " + shape + " shape " + ShapeArgs::INNER_RADIUS + " is required";
        }
        if (!existsAndNotEmptyString(geomArgs, ShapeArgs::OUTER_RADIUS)) {
          errors[flavour] = "For " + shape + " shape " + ShapeArgs::OUTER_RADIUS + " is required";
        }
        if (!existsAndNotEmptyString(geomArgs, ShapeArgs::HEIGHT)) {
          errors[flavour] = "For " + shape + " shape " + ShapeArgs::HEIGHT + " is required";
        }
      }
      if (shape == ShapeArgs::FLAT_PLATE_HOLDER) {
        if (!existsAndNotEmptyString(geomArgs, ShapeArgs::WIDTH)) {
          errors[flavour] = "For " + shape + " shape " + ShapeArgs::WIDTH + " is required";
        }
        if (!existsAndNotEmptyString(geomArgs, ShapeArgs::FRONT_THICK)) {
          errors[flavour] = "For " + shape + " shape " + ShapeArgs::FRONT_THICK + " is required";
        }
        if (!existsAndNotEmptyString(geomArgs, ShapeArgs::BACK_THICK)) {
          errors[flavour] = "For " + shape + " shape " + ShapeArgs::BACK_THICK + " is required";
        }
        if (!existsAndNotEmptyString(geomArgs, ShapeArgs::HEIGHT)) {
          errors[flavour] = "For " + shape + " shape " + ShapeArgs::HEIGHT + " is required";
        }
      }
      if (shape == ShapeArgs::HOLLOW_CYLINDER_HOLDER) {
        if (!existsAndNotEmptyString(geomArgs, ShapeArgs::INNER_OUTER_RADIUS)) {
          errors[flavour] = "For " + shape + " shape " + ShapeArgs::INNER_OUTER_RADIUS + " is required";
        }
        if (!existsAndNotEmptyString(geomArgs, ShapeArgs::OUTER_INNER_RADIUS)) {
          errors[flavour] = "For " + shape + " shape " + ShapeArgs::OUTER_INNER_RADIUS + " is required";
        }
        if (!existsAndNotEmptyString(geomArgs, ShapeArgs::HEIGHT)) {
          errors[flavour] = "For " + shape + " shape " + ShapeArgs::HEIGHT + " is required";
        }
      }
      if (shape == ShapeArgs::SPHERE) {
        if (!existsAndNotEmptyString(geomArgs, ShapeArgs::RADIUS)) {
          errors[flavour] = "For " + shape + " shape " + ShapeArgs::RADIUS + " is required";
        }
      }
    }
  } else {
    errors[flavour] = GeometryArgs::SHAPE + " is required";
  }
}

/**
 * @brief Validates the material
 * @param errors map
 * @param inputArgs material arguments
 * @param flavour sample or container
 */
void SetSample::validateMaterial(std::map<std::string, std::string> &errors, const Kernel::PropertyManager &inputArgs,
                                 const std::string &flavour) {
  PropertyManager args = materialSettingsEnsureLegacyCompatibility(inputArgs);
  ReadMaterial::MaterialParameters materialParams;
  setMaterial(materialParams, args);
  auto materialErrors = ReadMaterial::validateInputs(materialParams);
  if (!materialErrors.empty()) {
    std::stringstream ss;
    for (const auto &error : materialErrors) {
      ss << error.first << ":" << error.second << "\n";
    }
    errors[flavour] = ss.str();
  }
}

/**
 * @brief Ensures there is no specified property with negative value
 * @param errors map
 * @param geomArgs geometry arguments
 * @param flavour sample or container
 * @param keys the vector of property names to check
 */
void SetSample::assertNonNegative(std::map<std::string, std::string> &errors, const Kernel::PropertyManager &geomArgs,
                                  const std::string &flavour, const std::vector<const std::string *> &keys) {
  if (existsAndNotEmptyString(geomArgs, GeometryArgs::SHAPE)) {
    for (const auto &arg : keys) {
      if (existsAndNegative(geomArgs, *arg)) {
        errors[flavour] = *arg + " argument < 0.0";
      }
    }
  }
}

/**
 * @brief Checks if a json dictionary parameter is populated or not
 * @param dict map
 */
bool SetSample::isDictionaryPopulated(const PropertyManager_const_sptr &dict) const {
  bool isPopulated = false;
  if (dict)
    if (dict->propertyCount() > 0)
      isPopulated = true;
  return isPopulated;
}

/// Validate the inputs against each other @see Algorithm::validateInputs
std::map<std::string, std::string> SetSample::validateInputs() {
  std::map<std::string, std::string> errors;
  // Check workspace type has ExperimentInfo fields
  using API::ExperimentInfo_sptr;
  using API::Workspace_sptr;
  Workspace_sptr inputWS = getProperty(PropertyNames::INPUT_WORKSPACE);
  if (!std::dynamic_pointer_cast<ExperimentInfo>(inputWS)) {
    errors[PropertyNames::INPUT_WORKSPACE] = "InputWorkspace type invalid. "
                                             "Expected MatrixWorkspace, "
                                             "PeaksWorkspace.";
  }

  const PropertyManager_const_sptr geomArgs = getProperty(PropertyNames::GEOMETRY);

  const PropertyManager_const_sptr materialArgs = getProperty(PropertyNames::MATERIAL);

  const PropertyManager_const_sptr environArgs = getProperty(PropertyNames::ENVIRONMENT);

  const PropertyManager_const_sptr canGeomArgs = getProperty(PropertyNames::CONTAINER_GEOMETRY);

  const PropertyManager_const_sptr canMaterialArgs = getProperty(PropertyNames::CONTAINER_MATERIAL);

  const std::vector<const std::string *> positiveValues = {{&ShapeArgs::HEIGHT, &ShapeArgs::WIDTH, &ShapeArgs::THICK,
                                                            &ShapeArgs::RADIUS, &ShapeArgs::INNER_RADIUS,
                                                            &ShapeArgs::OUTER_RADIUS}};

  if (!isDictionaryPopulated(geomArgs) && !isDictionaryPopulated(materialArgs) && !isDictionaryPopulated(environArgs) &&
      !isDictionaryPopulated(canGeomArgs) && !isDictionaryPopulated(canMaterialArgs)) {
    errors["Geometry"] = "At least one of the input parameters must be populated";
  }

  if (isDictionaryPopulated(environArgs)) {
    if (!existsAndNotEmptyString(*environArgs, SEArgs::NAME)) {
      errors[PropertyNames::ENVIRONMENT] = "Environment flags require a non-empty 'Name' entry.";
    } else {
      // If specifying the environment through XML file, we can not strictly
      // validate the sample settings, since only the overriding properties
      // are specified. Hence we just make sure that whatever is specified is
      // at least positive
      if (isDictionaryPopulated(geomArgs)) {
        assertNonNegative(errors, *geomArgs, PropertyNames::GEOMETRY, positiveValues);
      }
    }
  } else {
    // We cannot strictly require geometry and material to be defined
    // simultaneously; it can be that one is defined at a later time
    if (isDictionaryPopulated(geomArgs)) {
      assertNonNegative(errors, *geomArgs, PropertyNames::GEOMETRY, positiveValues);
      validateGeometry(errors, *geomArgs, PropertyNames::GEOMETRY);
    }
    if (isDictionaryPopulated(materialArgs)) {
      validateMaterial(errors, *materialArgs, PropertyNames::MATERIAL);
    }
  }
  if (isDictionaryPopulated(canGeomArgs)) {
    assertNonNegative(errors, *canGeomArgs, PropertyNames::CONTAINER_GEOMETRY, positiveValues);
    validateGeometry(errors, *canGeomArgs, PropertyNames::CONTAINER_GEOMETRY);
  }

  if (isDictionaryPopulated(canMaterialArgs)) {
    validateMaterial(errors, *canMaterialArgs, PropertyNames::CONTAINER_MATERIAL);
  }
  return errors;
}

/**
 * Initialize the algorithm's properties.
 */
void SetSample::init() {
  using API::Workspace;
  using API::WorkspaceProperty;
  using Kernel::Direction;
  using Kernel::PropertyManagerProperty;

  // Inputs
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(PropertyNames::INPUT_WORKSPACE, "", Direction::InOut),
                  "A workspace whose sample properties will be updated");
  declareProperty(std::make_unique<PropertyManagerProperty>(PropertyNames::GEOMETRY, Direction::Input),
                  "A dictionary of geometry parameters for the sample.");
  declareProperty(std::make_unique<PropertyManagerProperty>(PropertyNames::MATERIAL, Direction::Input),
                  "A dictionary of material parameters for the sample. See "
                  "SetSampleMaterial for all accepted parameters");
  declareProperty(std::make_unique<PropertyManagerProperty>(PropertyNames::ENVIRONMENT, Direction::Input),
                  "A dictionary of parameters to configure the sample environment");
  declareProperty(std::make_unique<PropertyManagerProperty>(PropertyNames::CONTAINER_GEOMETRY, Direction::Input),
                  "A dictionary of geometry parameters for the container.");
  declareProperty(std::make_unique<PropertyManagerProperty>(PropertyNames::CONTAINER_MATERIAL, Direction::Input),
                  "A dictionary of material parameters for the container.");
}

/**
 * Execute the algorithm.
 */
void SetSample::exec() {
  using API::ExperimentInfo_sptr;
  using Kernel::PropertyManager_sptr;

  Workspace_sptr workspace = getProperty(PropertyNames::INPUT_WORKSPACE);
  PropertyManager_sptr environArgs = getProperty(PropertyNames::ENVIRONMENT);
  PropertyManager_sptr geometryArgs = getProperty(PropertyNames::GEOMETRY);
  PropertyManager_sptr materialArgs = getProperty(PropertyNames::MATERIAL);
  PropertyManager_sptr canGeometryArgs = getProperty(PropertyNames::CONTAINER_GEOMETRY);
  PropertyManager_sptr canMaterialArgs = getProperty(PropertyNames::CONTAINER_MATERIAL);

  // validateInputs guarantees this will be an ExperimentInfo object
  auto experimentInfo = std::dynamic_pointer_cast<ExperimentInfo>(workspace);
  // The order here is important. Set the environment first. If this
  // defines a sample geometry then we can process the Geometry flags
  // combined with this
  const SampleEnvironment *sampleEnviron(nullptr);
  if (isDictionaryPopulated(environArgs)) {
    sampleEnviron = setSampleEnvironmentFromFile(*experimentInfo, environArgs);
  } else if (isDictionaryPopulated(canGeometryArgs)) {
    setSampleEnvironmentFromXML(*experimentInfo, canGeometryArgs, canMaterialArgs);
  }

  double sampleVolume = 0.;
  if (isDictionaryPopulated(geometryArgs) || sampleEnviron) {
    setSampleShape(*experimentInfo, geometryArgs, sampleEnviron);
    if (experimentInfo->sample().getShape().hasValidShape()) {
      // get the volume back out to use in setting the material
      sampleVolume = CUBIC_METRE_TO_CM * experimentInfo->sample().getShape().volume();
    }
  }

  // Finally the material arguments
  if (isDictionaryPopulated(materialArgs)) {
    PropertyManager materialArgsCompatible = materialSettingsEnsureLegacyCompatibility(*materialArgs);
    // add the sample volume if it was defined/determined
    if (sampleVolume > 0.) {
      // only add the volume if it isn't already specfied
      if (!materialArgsCompatible.existsProperty("Volume")) {
        materialArgsCompatible.declareProperty(std::make_unique<PropertyWithValue<double>>("Volume", sampleVolume));
      }
    }
    // this does what SetSampleMaterial would do, but without calling it
    ReadMaterial::MaterialParameters materialParams;
    setMaterial(materialParams, materialArgsCompatible);
    ReadMaterial reader;
    reader.setMaterialParameters(materialParams);
    const auto sampleMaterial = reader.buildMaterial();
    auto shapeObject =
        std::shared_ptr<Geometry::IObject>(experimentInfo->sample().getShape().cloneWithMaterial(*sampleMaterial));
    experimentInfo->mutableSample().setShape(shapeObject);
  }
}

/**
 * Set the requested sample environment on the workspace from the environment
 * file
 * @param exptInfo A reference to the ExperimentInfo to receive the environment
 * @param args The dictionary of flags for the environment
 * @return A pointer to the new sample environment
 */
const Geometry::SampleEnvironment *
SetSample::setSampleEnvironmentFromFile(API::ExperimentInfo &exptInfo, const Kernel::PropertyManager_const_sptr &args) {
  using Kernel::ConfigService;

  const std::string envName = args->getPropertyValue(SEArgs::NAME);
  std::string canName = "";
  if (args->existsProperty(SEArgs::CONTAINER)) {
    canName = args->getPropertyValue(SEArgs::CONTAINER);
  }
  // The specifications need to be qualified by the facility and instrument.
  // Check instrument for name and then lookup facility if facility
  // is unknown then set to default facility & instrument.
  auto instrument = exptInfo.getInstrument();
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
  auto finder = std::make_unique<SampleEnvironmentSpecFileFinder>(environDirs);
  SampleEnvironmentFactory factory(std::move(finder));
  Geometry::SampleEnvironment_uptr sampleEnviron;
  if (args->existsProperty(SEArgs::PATH)) {
    auto sampleEnvironSpec = factory.parseSpec(envName, args->getPropertyValue(SEArgs::PATH));
    sampleEnviron = sampleEnvironSpec->buildEnvironment(canName);
  } else {
    sampleEnviron = factory.create(facilityName, instrumentName, envName, canName);
  }
  exptInfo.mutableSample().setEnvironment(std::move(sampleEnviron));
  return &(exptInfo.sample().getEnvironment());
}

/**
 * Set the requested sample environment from shape XML string
 * @param exptInfo A reference to the ExperimentInfo to receive the environment
 * @param canGeomArgs The dictionary of flags for the environment
 * @param canMaterialArgs The dictionary of material parameters
 * @return A pointer to the new sample environment
 */
const Geometry::SampleEnvironment *
SetSample::setSampleEnvironmentFromXML(API::ExperimentInfo &exptInfo,
                                       const Kernel::PropertyManager_const_sptr &canGeomArgs,
                                       const Kernel::PropertyManager_const_sptr &canMaterialArgs) {
  const auto refFrame = exptInfo.getInstrument()->getReferenceFrame();
  const auto xml = tryCreateXMLFromArgsOnly(*canGeomArgs, *refFrame);
  if (!xml.empty()) {
    ShapeFactory sFactory;
    // Create the object
    auto shape = sFactory.createShape(xml);
    if (shape->hasValidShape()) {
      if (canMaterialArgs) {
        PropertyManager canMaterialCompatible = materialSettingsEnsureLegacyCompatibility(*canMaterialArgs);
        ReadMaterial::MaterialParameters materialParams;
        setMaterial(materialParams, canMaterialCompatible);
        if (materialParams.volume <= 0.) {
          materialParams.volume = shape->volume() * CUBIC_METRE_TO_CM;
        }
        ReadMaterial reader;
        reader.setMaterialParameters(materialParams);
        auto canMaterial = reader.buildMaterial();
        shape->setMaterial(*canMaterial);
      }
      const SampleEnvironment se("unnamed", std::make_shared<Container>(shape));
      exptInfo.mutableSample().setEnvironment(std::make_unique<SampleEnvironment>(se));
    }
  }
  return &(exptInfo.sample().getEnvironment());
}

/**
 * @brief SetSample::setMaterial Configures a material from the parameters
 * @param materialParams : output material parameters object
 * @param materialArgs : input material arguments, can be altered (see comment
 * inside)
 */
void SetSample::setMaterial(ReadMaterial::MaterialParameters &materialParams,
                            const Kernel::PropertyManager &materialArgs) {
  if (materialArgs.existsProperty("ChemicalFormula")) {
    materialParams.chemicalSymbol = materialArgs.getPropertyValue("ChemicalFormula");
  }
  if (materialArgs.existsProperty("AtomicNumber")) {
    materialParams.atomicNumber = materialArgs.getProperty("AtomicNumber");
  }
  if (materialArgs.existsProperty("MassNumber")) {
    materialParams.massNumber = materialArgs.getProperty("MassNumber");
  }
  if (materialArgs.existsProperty("CoherentXSection")) {
    materialParams.coherentXSection = materialArgs.getProperty("CoherentXSection");
  }
  if (materialArgs.existsProperty("IncoherentXSection")) {
    materialParams.incoherentXSection = materialArgs.getProperty("IncoherentXSection");
  }
  if (materialArgs.existsProperty("AttenuationXSection")) {
    materialParams.attenuationXSection = materialArgs.getProperty("AttenuationXSection");
  }
  if (materialArgs.existsProperty("ScatteringXSection")) {
    materialParams.scatteringXSection = materialArgs.getProperty("ScatteringXSection");
  }
  if (materialArgs.existsProperty("NumberDensityUnit")) {
    const std::string numberDensityUnit = materialArgs.getProperty("NumberDensityUnit");
    if (numberDensityUnit == "Atoms") {
      materialParams.numberDensityUnit = MaterialBuilder::NumberDensityUnit::Atoms;
    } else {
      materialParams.numberDensityUnit = MaterialBuilder::NumberDensityUnit::FormulaUnits;
    }
  }
  if (materialArgs.existsProperty("ZParameter")) {
    materialParams.zParameter = materialArgs.getProperty("ZParameter");
  }
  if (materialArgs.existsProperty("UnitCellVolume")) {
    materialParams.unitCellVolume = materialArgs.getProperty("UnitCellVolume");
  }
  if (materialArgs.existsProperty("NumberDensity")) {
    materialParams.numberDensity = materialArgs.getProperty("NumberDensity");
  }
  if (materialArgs.existsProperty("MassDensity")) {
    materialParams.massDensity = materialArgs.getProperty("MassDensity");
  }
  if (materialArgs.existsProperty("EffectiveNumberDensity")) {
    materialParams.numberDensityEffective = materialArgs.getProperty("EffectiveNumberDensity");
  }
  if (materialArgs.existsProperty("PackingFraction")) {
    materialParams.packingFraction = materialArgs.getProperty("packingFraction");
  }
  if (materialArgs.existsProperty("Mass")) {
    materialParams.mass = materialArgs.getProperty("Mass");
  }
  if (materialArgs.existsProperty("Volume")) {
    materialParams.volume = materialArgs.getProperty("Volume");
  }
}

/**
 * @param experiment A reference to the experiment to be affected
 * @param args The user-supplied dictionary of flags
 * @param sampleEnv A pointer to the sample environment if one exists, otherwise null
 */
void SetSample::setSampleShape(API::ExperimentInfo &experiment, const Kernel::PropertyManager_const_sptr &args,
                               const Geometry::SampleEnvironment *sampleEnv) {
  using Geometry::Container;
  /* The sample geometry can be specified in two ways:
     - a known set of primitive shapes with values or CSG string
     - or a <samplegeometry> field sample environment can, with values possible
       overridden by the Geometry flags
  */

  // Try known shapes or CSG first if supplied
  if (isDictionaryPopulated(args)) {
    const auto refFrame = experiment.getInstrument()->getReferenceFrame();
    auto xml = tryCreateXMLFromArgsOnly(*args, *refFrame);
    if (!xml.empty()) {
      Kernel::Matrix<double> rotationMatrix = experiment.run().getGoniometer().getR();
      if (rotationMatrix != Kernel::Matrix<double>(3, 3, true) && !sampleEnv) {
        // Only add goniometer tag if rotationMatrix is not the Identity,
        // and this shape is not defined within a sample environment
        xml = Geometry::ShapeFactory().addGoniometerTag(rotationMatrix, xml);
      }
      CreateSampleShape::setSampleShape(experiment, xml);
      return;
    }
  }
  // Any arguments in the args dict are assumed to be values that should
  // override the default set by the sampleEnv samplegeometry if it exists
  if (sampleEnv) {
    const auto &can = sampleEnv->getContainer();
    if (sampleEnv->getContainer().hasCustomizableSampleShape()) {
      Container::ShapeArgs shapeArgs;
      if (isDictionaryPopulated(args)) {
        const auto &props = args->getProperties();
        for (const auto &prop : props) {
          // assume in cm
          const double val = getPropertyAsDouble(*args, prop->name());
          shapeArgs.emplace(boost::algorithm::to_lower_copy(prop->name()), val * 0.01);
        }
      }
      auto shapeObject = can.createSampleShape(shapeArgs);
      // Given that the object is a CSG object, set the object
      // directly on the sample ensuring we preserve the
      // material.
      const auto mat = experiment.sample().getMaterial();
      if (auto csgObj = std::dynamic_pointer_cast<Geometry::CSGObject>(shapeObject)) {
        csgObj->setMaterial(mat);
      }
      experiment.mutableSample().setShape(shapeObject);
    } else if (sampleEnv->getContainer().hasFixedSampleShape()) {
      if (isDictionaryPopulated(args)) {
        throw std::runtime_error("The can has a fixed sample shape that cannot "
                                 "be adjusted using the Geometry parameter.");
      }
      auto shapeObject = can.getSampleShape();

      // apply Goniometer rotation
      // Rotate only implemented on mesh objects so far
      if (typeid(shapeObject) == typeid(std::shared_ptr<Geometry::MeshObject>)) {
        const std::vector<double> rotationMatrix = experiment.run().getGoniometer().getR();
        std::dynamic_pointer_cast<Geometry::MeshObject>(shapeObject)->rotate(rotationMatrix);
      }

      const auto mat = experiment.sample().getMaterial();
      shapeObject->setMaterial(mat);

      experiment.mutableSample().setShape(shapeObject);
    } else {
      if (isDictionaryPopulated(args)) {
        throw std::runtime_error("Cannot override the sample shape because the "
                                 "environment definition does not define a "
                                 "default sample shape. Please either provide "
                                 "a 'Shape' argument in the dictionary for the "
                                 "Geometry parameter or update the environment "
                                 "definition with this information.");
      }
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
std::string SetSample::tryCreateXMLFromArgsOnly(const Kernel::PropertyManager &args,
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
  } else if (shape.ends_with(ShapeArgs::CYLINDER)) {
    result = createCylinderLikeXML(args, refFrame, boost::algorithm::equals(shape, ShapeArgs::HOLLOW_CYLINDER));
  } else if (shape.ends_with(ShapeArgs::FLAT_PLATE_HOLDER)) {
    result = createFlatPlateHolderXML(args, refFrame);
  } else if (shape.ends_with(ShapeArgs::HOLLOW_CYLINDER_HOLDER)) {
    result = createHollowCylinderHolderXML(args, refFrame);
  } else if (shape.ends_with(ShapeArgs::SPHERE)) {
    result = createSphereXML(args);
  } else {
    std::stringstream msg;
    msg << "Unknown 'Shape' argument '" << shape << "' provided in 'Geometry' property. Allowed values are "
        << ShapeArgs::CSG << ", " << ShapeArgs::FLAT_PLATE << ", " << ShapeArgs::CYLINDER << ", "
        << ShapeArgs::HOLLOW_CYLINDER << ", " << ShapeArgs::FLAT_PLATE_HOLDER << ", "
        << ShapeArgs::HOLLOW_CYLINDER_HOLDER << ", " << ShapeArgs::SPHERE;
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
 * @param id A generic id for the shape element
 * @return The XML definition string
 */
std::string SetSample::createFlatPlateXML(const Kernel::PropertyManager &args, const Geometry::ReferenceFrame &refFrame,
                                          const std::string &id) const {
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
  if (args.existsProperty(ShapeArgs::ANGLE)) {
    const double angleInDegrees = getPropertyAsDouble(args, ShapeArgs::ANGLE);
    Goniometer gr;
    const auto upAxis = makeV3D(0, 1, 0);
    gr.pushAxis("up", upAxis.X(), upAxis.Y(), upAxis.Z(), angleInDegrees, Geometry::CCW, Geometry::angDegrees);
    auto &rotation = gr.getR();
    lfb.rotate(rotation);
    lft.rotate(rotation);
    lbb.rotate(rotation);
    rfb.rotate(rotation);
  }
  std::vector<double> center = {0., 0., 0.};
  if (args.existsProperty(ShapeArgs::CENTER)) {
    center = getPropertyAsVectorDouble(args, ShapeArgs::CENTER);
    const V3D centrePos(center[0] * 0.01, center[1] * 0.01, center[2] * 0.01);
    // translate to true center after rotation
    lfb += centrePos;
    lft += centrePos;
    lbb += centrePos;
    rfb += centrePos;
  }
  std::ostringstream xmlShapeStream;
  xmlShapeStream << " <cuboid id=\"" << id << "\"> "
                 << "<left-front-bottom-point x=\"" << lfb.X() << "\" y=\"" << lfb.Y() << "\" z=\"" << lfb.Z()
                 << "\"  /> "
                 << "<left-front-top-point  x=\"" << lft.X() << "\" y=\"" << lft.Y() << "\" z=\"" << lft.Z()
                 << "\"  /> "
                 << "<left-back-bottom-point  x=\"" << lbb.X() << "\" y=\"" << lbb.Y() << "\" z=\"" << lbb.Z()
                 << "\"  /> "
                 << "<right-front-bottom-point  x=\"" << rfb.X() << "\" y =\"" << rfb.Y() << "\" z=\"" << rfb.Z()
                 << "\"  /> "
                 << "</cuboid>";
  return xmlShapeStream.str();
}

/**
 * Create the XML required to define a flat plate holder from the given args
 * Flate plate holder is a CSG union of 2 flat plates one on each side of the
 * sample
 * The front and back holders are supposed to have the same width and height and
 * angle as the sample Only the centre needs to be calculated taking into
 * account the thichkness of the sample in between
 * @param args A user-supplied dict of args
 * @param refFrame Defines the reference frame for the shape
 * @return The XML definition string
 */
std::string SetSample::createFlatPlateHolderXML(const Kernel::PropertyManager &args,
                                                const Geometry::ReferenceFrame &refFrame) const {

  std::vector<double> centre = {0., 0., 0.};
  if (args.existsProperty(ShapeArgs::CENTER)) {
    centre = getPropertyAsVectorDouble(args, ShapeArgs::CENTER);
  }

  const double sampleThickness = getPropertyAsDouble(args, ShapeArgs::THICK);
  const double frontPlateThickness = getPropertyAsDouble(args, ShapeArgs::FRONT_THICK);
  const double backPlateThickness = getPropertyAsDouble(args, ShapeArgs::BACK_THICK);
  double angle = 0.;
  if (args.existsProperty(ShapeArgs::ANGLE)) {
    angle = degToRad(getPropertyAsDouble(args, ShapeArgs::ANGLE));
  }
  const auto pointingAlongBeam = refFrame.pointingAlongBeam();
  const auto pointingHorizontal = refFrame.pointingHorizontal();
  const auto handedness = refFrame.getHandedness();
  const int signHorizontal = (handedness == Mantid::Geometry::Handedness::Right) ? 1 : -1;

  auto frontPlate = args;
  frontPlate.setProperty(ShapeArgs::THICK, frontPlateThickness);
  auto frontCentre = centre;
  const double frontCentreOffset = (frontPlateThickness + sampleThickness) * 0.5;
  frontCentre[pointingAlongBeam] -= frontCentreOffset * std::cos(angle);
  frontCentre[pointingHorizontal] -= signHorizontal * frontCentreOffset * std::sin(angle);
  if (!frontPlate.existsProperty(ShapeArgs::CENTER)) {
    frontPlate.declareProperty(ShapeArgs::CENTER, frontCentre);
  }
  frontPlate.setProperty(ShapeArgs::CENTER, frontCentre);
  const std::string frontPlateXML = createFlatPlateXML(frontPlate, refFrame, "front");

  auto backPlate = args;
  backPlate.setProperty(ShapeArgs::THICK, backPlateThickness);
  auto backCentre = centre;
  const double backCentreOffset = (backPlateThickness + sampleThickness) * 0.5;
  backCentre[pointingAlongBeam] += backCentreOffset * std::cos(angle);
  backCentre[pointingHorizontal] += signHorizontal * backCentreOffset * std::sin(angle);
  if (!backPlate.existsProperty(ShapeArgs::CENTER)) {
    backPlate.declareProperty(ShapeArgs::CENTER, backCentre);
  }
  backPlate.setProperty(ShapeArgs::CENTER, backCentre);
  const std::string backPlateXML = createFlatPlateXML(backPlate, refFrame, "back");

  return frontPlateXML + backPlateXML + "<algebra val=\"back:front\"/>";
}

/**
 * Create the XML required to define a hollow cylinder holder from the given
 * args Hollow cylinder holder is a CSG union of 2 hollow cylinders one inside,
 * one outside the sample The centre, the axis and the height are assumed to be
 * the same as for the sample Only the inner and outer radii need to be
 * manipulated
 * @param args A user-supplied dict of args
 * @param refFrame Defines the reference frame for the shape
 * @return The XML definition string
 */
std::string SetSample::createHollowCylinderHolderXML(const Kernel::PropertyManager &args,
                                                     const Geometry::ReferenceFrame &refFrame) const {
  auto innerCylinder = args;
  const double innerOuterRadius = getPropertyAsDouble(args, ShapeArgs::INNER_OUTER_RADIUS);
  innerCylinder.setProperty(ShapeArgs::OUTER_RADIUS, innerOuterRadius);
  const std::string innerCylinderXML = createCylinderLikeXML(innerCylinder, refFrame, true, "inner");
  auto outerCylinder = args;
  const double outerInnerRadius = getPropertyAsDouble(args, ShapeArgs::OUTER_INNER_RADIUS);
  outerCylinder.setProperty(ShapeArgs::INNER_RADIUS, outerInnerRadius);
  const std::string outerCylinderXML = createCylinderLikeXML(outerCylinder, refFrame, true, "outer");
  return innerCylinderXML + outerCylinderXML + "<algebra val=\"inner:outer\"/>";
}

/**
 * Create the XML required to define a cylinder from the given args
 * @param args A user-supplied dict of args
 * @param refFrame Defines the reference frame for the shape
 * @param hollow True if an annulus is to be created
 * @param id A generic id for the shape element
 * @return The XML definition string
 */
std::string SetSample::createCylinderLikeXML(const Kernel::PropertyManager &args,
                                             const Geometry::ReferenceFrame &refFrame, bool hollow,
                                             const std::string &id) const {
  const std::string tag = hollow ? "hollow-cylinder" : "cylinder";
  double height = getPropertyAsDouble(args, ShapeArgs::HEIGHT);
  double innerRadius = hollow ? getPropertyAsDouble(args, ShapeArgs::INNER_RADIUS) : 0.0;
  double outerRadius =
      hollow ? getPropertyAsDouble(args, ShapeArgs::OUTER_RADIUS) : getPropertyAsDouble(args, "Radius");
  std::vector<double> centre = {0., 0., 0.};
  if (args.existsProperty(ShapeArgs::CENTER)) {
    centre = getPropertyAsVectorDouble(args, ShapeArgs::CENTER);
    std::transform(centre.begin(), centre.end(), centre.begin(), [](double val) { return val *= 0.01; });
  }
  // convert to metres
  height *= 0.01;
  innerRadius *= 0.01;
  outerRadius *= 0.01;
  // XML needs center position of bottom base but user specifies center of
  // cylinder
  V3D baseCentre;
  std::ostringstream XMLString;
  if (args.existsProperty(ShapeArgs::AXIS)) {
    const std::string axis = args.getPropertyValue(ShapeArgs::AXIS);
    if (axis.length() == 1) {
      const auto axisId = static_cast<unsigned>(std::stoi(axis));
      XMLString << axisXML(axisId);
      baseCentre = cylBaseCentre(centre, height, axisId);
    } else {
      const std::vector<double> axisVector = getPropertyAsVectorDouble(args, ShapeArgs::AXIS);
      XMLString << axisXML(axisVector);
      baseCentre = cylBaseCentre(centre, height, axisVector);
    }
  } else {
    const auto axisId = static_cast<unsigned>(refFrame.pointingUp());
    XMLString << axisXML(axisId);
    baseCentre = cylBaseCentre(centre, height, axisId);
  }

  std::ostringstream xmlShapeStream;
  xmlShapeStream << "<" << tag << " id=\"" << id << "\"> "
                 << "<centre-of-bottom-base x=\"" << baseCentre.X() << "\" y=\"" << baseCentre.Y() << "\" z=\""
                 << baseCentre.Z() << "\" /> " << XMLString.str() << "<height val=\"" << height << "\" /> ";
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
 * Create the XML required to define a sphere from the given args
 * @param args A user-supplied dict of args
 * @return The XML definition string
 */
std::string SetSample::createSphereXML(const Kernel::PropertyManager &args) const {
  const double radius = static_cast<double>(args.getProperty(ShapeArgs::RADIUS)) * 0.01;
  std::vector<double> center = getPropertyAsVectorDouble(args, ShapeArgs::CENTER);
  std::transform(center.begin(), center.end(), center.begin(), [](double val) { return val *= 0.01; });

  std::ostringstream xmlShapeStream;
  const std::string tag{"sphere"};
  const auto id = "sphere";
  xmlShapeStream << "<" << tag << " id=\"" << id << "\"> "
                 << "<center x=\"" << center[0] << "\" y=\"" << center[1] << "\" z=\"" << center[2] << "\" /> "
                 << "<radius val=\"" << radius << "\" /> ";
  xmlShapeStream << "</" << tag << ">";
  return xmlShapeStream.str();
}

/**
 * @brief Ensures the backwards compatibility of material arguments
 * @param materialArgs const input material arguments
 * @return copy of material arguments without Sample prefix (see the comment)
 */
PropertyManager SetSample::materialSettingsEnsureLegacyCompatibility(const PropertyManager &materialArgs) {
  PropertyManager compatible(materialArgs);

  // The material should be agnostic whether it's sample's material or
  // container's so in the properties below there should be no sample prefix,
  // i.e. NumberDensity instead of SampleNumberDensity.
  // However, for legacy compatibility, those prefixed with Sample are still
  // considered through aliases
  if (materialArgs.existsProperty("SampleNumberDensity")) {
    const double numberDensity = materialArgs.getProperty("SampleNumberDensity");
    if (!compatible.existsProperty("NumberDensity")) {
      compatible.declareProperty("NumberDensity", numberDensity);
    } else {
      compatible.setProperty("NumberDensity", numberDensity);
    }
  }
  if (materialArgs.existsProperty("SampleEffectiveNumberDensity")) {
    const double numberDensityEff = materialArgs.getProperty("SampleEffectiveNumberDensity");
    if (!compatible.existsProperty("EffectiveNumberDensity")) {
      compatible.declareProperty("EffectiveNumberDensity", numberDensityEff);
    } else {
      compatible.setProperty("EffectiveNumberDesnity", numberDensityEff);
    }
  }
  if (materialArgs.existsProperty("SamplePackingFraction")) {
    const double packingFraction = materialArgs.getProperty("SamplePackingFraction");
    if (!compatible.existsProperty("PackingFraction")) {
      compatible.declareProperty("PackingFraction", packingFraction);
    } else {
      compatible.setProperty("PackingFraction", packingFraction);
    }
  }
  if (materialArgs.existsProperty("SampleMassDensity")) {
    const double massDensity = materialArgs.getProperty("SampleMassDensity");
    if (!compatible.existsProperty("MassDensity")) {
      compatible.declareProperty("MassDensity", massDensity);
    } else {
      compatible.setProperty("MassDensity", massDensity);
    }
  }
  if (materialArgs.existsProperty("SampleMass")) {
    const double mass = materialArgs.getProperty("SampleMass");
    if (!compatible.existsProperty("Mass")) {
      compatible.declareProperty("Mass", mass);
    } else {
      compatible.setProperty("Mass", mass);
    }
  }
  if (materialArgs.existsProperty("SampleVolume")) {
    const double volume = materialArgs.getProperty("SampleVolume");
    if (!compatible.existsProperty("Volume")) {
      compatible.declareProperty("Volume", volume);
    } else {
      compatible.setProperty("Volume", volume);
    }
  }
  return compatible;
}
} // namespace Mantid::DataHandling
