#include "MantidDataHandling/SetSample.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/SampleEnvironmentFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/PropertyManagerProperty.h"

#include <Poco/Path.h>

namespace Mantid {
namespace DataHandling {

using Geometry::SampleEnvironment;

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

    if (!environArgs->existsProperty("Can")) {
      errors["Environment"] = "Environment flags must contain a 'Can' entry.";
    } else {
      std::string name = environArgs->getPropertyValue("Can");
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
  const std::string canName = args.getPropertyValue("Can");
  // The specifications need to be qualified by the facility and instrument.
  //  - check instrument for name and then lookup facility
  auto instrument = workspace->getInstrument();
  const auto &instName = instrument->getName();
  const auto &config = ConfigService::Instance();
  const auto &instInfo = config.getInstrument(instName);
  const auto &facilityInfo = instInfo.facility();

  const auto &instDirs = config.getInstrumentDirectories();
  std::vector<std::string> environDirs(instDirs);
  for (auto &direc : environDirs) {
    direc = Poco::Path(direc).append("sampleenvironments").toString();
  }
  auto finder =
      Kernel::make_unique<SampleEnvironmentSpecFileFinder>(environDirs);
  SampleEnvironmentFactory factory(std::move(finder));
  auto sampleEnviron =
      factory.create(facilityInfo.name(), instInfo.name(), envName, canName);
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
  using Geometry::Can;
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
    if (sampleEnv->can()->hasSampleShape()) {
      const auto &can = sampleEnv->can();
      Can::ShapeArgs shapeArgs;
      if (args) {
        const auto &props = args->getProperties();
        for (const auto &prop : props) {
          const double val = args->getProperty(prop->name());
          shapeArgs.insert(std::make_pair(prop->name(), val));
        }
      }
      auto shapeObject = can->createSampleShape(shapeArgs);
      // Set the object directly on the sample ensuring we preserve the
      // material
      const auto &mat = workspace->sample().getMaterial();
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
  } else if (shape == "Cylinder") {
    result = createCylinderXML(*args);
  } else if (shape == "Annulus") {
    result = createAnnulusXML(*args);
  } else {
    throw std::invalid_argument("Unknown 'Shape' argument provided in "
                                "'Geometry'. Allowed "
                                "values=FlatPlate,CSG,Cylinder,Annulus.");
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
  throw std::runtime_error("Not implemented");
}

/**
 * Create the XML required to define a cylinder from the given args
 * @param args A user-supplied dict of args
 * @return The XML definition string
 */
std::string
SetSample::createCylinderXML(const Kernel::PropertyManager &args) const {
  throw std::runtime_error("Not implemented");
}

/**
 * Create the XML required to define an annulus from the given args
 * @param args A user-supplied dict of args
 * @return The XML definition string
 */
std::string
SetSample::createAnnulusXML(const Kernel::PropertyManager &args) const {
  throw std::runtime_error("Not implemented");
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
