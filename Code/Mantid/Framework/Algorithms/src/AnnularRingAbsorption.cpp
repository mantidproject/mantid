#include "MantidAlgorithms/AnnularRingAbsorption.h"

#include "MantidAPI/SampleEnvironment.h"
#include "MantidAPI/WorkspaceValidators.h"

#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Objects/ShapeFactory.h"

#include "MantidKernel/Atom.h"
#include "MantidKernel/NeutronAtom.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/V3D.h"

#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace Algorithms {
using namespace Mantid::API;
using Mantid::Geometry::ObjComponent;
using namespace Mantid::Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AnnularRingAbsorption)

//----------------------------------------------------------------------------------------------
/** Constructor
*/
AnnularRingAbsorption::AnnularRingAbsorption() {}

//----------------------------------------------------------------------------------------------
/** Destructor
*/
AnnularRingAbsorption::~AnnularRingAbsorption() {}

//----------------------------------------------------------------------------------------------

/// Algorithm's name for identification. @see Algorithm::version
const std::string AnnularRingAbsorption::name() const {
  return "AnnularRingAbsorption";
}

/// Algorithm's version for identification. @see Algorithm::version
int AnnularRingAbsorption::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string AnnularRingAbsorption::category() const {
  return "CorrectionFunctions\\AbsorptionCorrections";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string AnnularRingAbsorption::summary() const {
  return "Calculates bin-by-bin correction factors for attenuation due to "
         "absorption "
         "in a cylindrical sample in the wall of a hollow can";
}

//----------------------------------------------------------------------------------------------
/**
 * Initialize the algorithm's properties.
 */
void AnnularRingAbsorption::init() {
  // The input workspace must have an instrument and units of wavelength
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<InstrumentValidator>();
  declareProperty(new WorkspaceProperty<>("InputWorkspace", "",
                                          Direction::Input, wsValidator),
                  "The input workspace in units of wavelength.");

  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "The name to use for the output workspace.");

  // -- can properties --
  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("CanOuterRadius", -1.0, mustBePositive,
                  "The outer radius of the can in centimetres");
  declareProperty("CanInnerRadius", -1.0, mustBePositive,
                  "The inner radius of the can in centimetres");

  // -- sample properties --
  declareProperty("SampleHeight", -1.0, mustBePositive,
                  "The height of the sample in centimetres");
  declareProperty("SampleThickness", -1.0, mustBePositive,
                  "The thickness of the sample in centimetres");
  auto nonEmptyString = boost::make_shared<MandatoryValidator<std::string>>();
  declareProperty("SampleChemicalFormula", "",
                  "Chemical composition of the sample material",
                  nonEmptyString);
  declareProperty("SampleNumberDensity", -1.0, mustBePositive,
                  "The number density of the sample in number of formulas per "
                  "cubic angstrom");

  // -- Monte Carlo properties --
  auto positiveInt = boost::make_shared<Kernel::BoundedValidator<int>>();
  positiveInt->setLower(1);
  declareProperty("NumberOfWavelengthPoints", EMPTY_INT(), positiveInt,
                  "The number of wavelength points for which a simulation is "
                  "atttempted (default: all points)");
  declareProperty(
      "EventsPerPoint", 300, positiveInt,
      "The number of \"neutron\" events to generate per simulated point");
  declareProperty("SeedValue", 123456789, positiveInt,
                  "Seed the random number generator with this value");
}

//----------------------------------------------------------------------------------------------
/**
 * Execute the algorithm.
 */
void AnnularRingAbsorption::exec() {
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  // We neglect any absorption in the can so the actual shape defined is a
  // hollow cylinder
  // where the sample is in the centre of the can wall
  attachSample(inputWS);

  MatrixWorkspace_sptr factors = runMonteCarloAbsorptionCorrection(inputWS);
  setProperty("OutputWorkspace", factors);
}

//---------------------------------------------------------------------------------------------
// Private members
//---------------------------------------------------------------------------------------------

/**
 * @param workspace The workspace where the environment should be attached
 */
void AnnularRingAbsorption::attachSample(MatrixWorkspace_sptr &workspace) {
  runCreateSampleShape(workspace);
  runSetSampleMaterial(workspace);
}

/**
 * @return Creates a new shape object for the sample
 */
void AnnularRingAbsorption::runCreateSampleShape(
    API::MatrixWorkspace_sptr &workspace) {
  auto inst = workspace->getInstrument();
  auto refFrame = inst->getReferenceFrame();

  bool childLog = g_log.is(Logger::Priority::PRIO_DEBUG);
  auto alg = this->createChildAlgorithm("CreateSampleShape", -1, -1, childLog);
  alg->setProperty("InputWorkspace", workspace);
  alg->setPropertyValue("ShapeXML",
                        createSampleShapeXML(refFrame->vecPointingUp()));
  try {
    alg->executeAsChildAlg();
  } catch (std::exception &exc) {
    throw std::invalid_argument(
        std::string("Unable to create sample shape: '") + exc.what() + "'");
  }
}

/**
 * Create the XML that defines a hollow cylinder with dimensions provided by the
 * user
 * The shape is a hollow cylinder where the inner/outer radius is defined by
 * \f[
 *    r_{\pm} = r_l + \frac{r_u-r_l}{2} \pm \frac{t}{2}
 * \f]
 * where \f$r_{l,u}\f$ are the inner & outer can radii respectively and \f$t\f$
 * is the sample
 * thickness
 * @param upAxis A vector pointing up
 * @returns A string containing the shape XML
 */
std::string
AnnularRingAbsorption::createSampleShapeXML(const V3D &upAxis) const {
  // User input
  const double canLowRadiusCM = getProperty("CanInnerRadius");
  const double canUppRadiusCM = getProperty("CanOuterRadius");
  const double sampleHeightCM = getProperty("SampleHeight");
  const double sampleThickCM = getProperty("SampleThickness");
  // Sample dimensions for approximation (converted to metres)
  const double wallMidPtCM =
      canLowRadiusCM + 0.5 * (canUppRadiusCM - canLowRadiusCM);
  const double lowRadiusMtr = (wallMidPtCM - 0.5 * sampleThickCM) / 100.;
  const double uppRadiusMtr = (wallMidPtCM + 0.5 * sampleThickCM) / 100.;

  // Cylinders oriented along Y, with origin at centre of bottom base
  const std::string innerCylID = std::string("inner-cyl");
  const std::string innerCyl = cylinderXML(innerCylID, V3D(), lowRadiusMtr,
                                           upAxis, sampleHeightCM / 100.0);
  const std::string outerCylID = std::string("outer-cyl");
  const std::string outerCyl = cylinderXML(outerCylID, V3D(), uppRadiusMtr,
                                           upAxis, sampleHeightCM / 100.0);

  // Combine shapes
  boost::format algebra("<algebra val=\"(%1% (# %2%))\" />");
  algebra % outerCylID % innerCylID;
  auto xml = outerCyl + "\n" + innerCyl + "\n" + algebra.str();
  g_log.debug() << "Sample shape XML:\n" << xml << "\n";
  return xml;
}

/**
 * @param id String id of object
 * @param bottomCentre Point of centre of bottom base
 * @param radius Radius of cylinder
 * @param axis Cylinder will point along this axis
 * @param height The height of the cylinder
 * @return A string defining the XML
 */
const std::string
AnnularRingAbsorption::cylinderXML(const std::string &id,
                                   const V3D &bottomCentre, const double radius,
                                   const V3D &axis, const double height) const {
  // The newline characters are not necessary for the XML but they make it
  // easier to read for debugging
  static const char *CYL_TEMPLATE =
      "<cylinder id=\"%1%\">\n"
      "<centre-of-bottom-base x=\"%2%\" y=\"%3%\" z=\"%4%\" />\n"
      " <axis x=\"%5%\" y=\"%6%\" z=\"%7%\" />\n"
      " <radius val=\"%8%\" />\n"
      " <height val=\"%9%\" />\n"
      "</cylinder>";

  boost::format xml(CYL_TEMPLATE);
  xml % id % bottomCentre.X() % bottomCentre.Y() % bottomCentre.Z() % axis.X() %
      axis.Y() % axis.Z() % radius % height;
  return xml.str();
}

/**
 * @return Attaches a new Material object to the sample
 */
void AnnularRingAbsorption::runSetSampleMaterial(
    API::MatrixWorkspace_sptr &workspace) {
  bool childLog = g_log.is(Logger::Priority::PRIO_DEBUG);
  auto alg = this->createChildAlgorithm("SetSampleMaterial", -1, -1, childLog);
  alg->setProperty("InputWorkspace", workspace);
  alg->setProperty("ChemicalFormula",
                   getPropertyValue("SampleChemicalFormula"));
  alg->setProperty<double>("SampleNumberDensity",
                           getProperty("SampleNumberDensity"));
  try {
    alg->executeAsChildAlg();
  } catch (std::exception &exc) {
    throw std::invalid_argument(
        std::string("Unable to set sample material: '") + exc.what() + "'");
  }
}

/**
 * Run the MonteCarloAbsorption algorithm on the given workspace and return the
 * calculated factors
 * @param workspace The input workspace that has the sample and can defined
 * @return A 2D workspace of attenuation factors
 */
MatrixWorkspace_sptr AnnularRingAbsorption::runMonteCarloAbsorptionCorrection(
    const MatrixWorkspace_sptr &workspace) {
  bool childLog = g_log.is(Logger::Priority::PRIO_DEBUG);
  auto alg =
      this->createChildAlgorithm("MonteCarloAbsorption", 0.1, 1.0, childLog);
  alg->setProperty("InputWorkspace", workspace);
  alg->setProperty<int>("NumberOfWavelengthPoints",
                        getProperty("NumberOfWavelengthPoints"));
  alg->setProperty<int>("EventsPerPoint", getProperty("EventsPerPoint"));
  alg->setProperty<int>("SeedValue", getProperty("SeedValue"));
  try {
    alg->executeAsChildAlg();
  } catch (std::exception &exc) {
    throw std::invalid_argument(
        std::string("Error running absorption correction: '") + exc.what() +
        "'");
  }

  return alg->getProperty("OutputWorkspace");
}

} // namespace Algorithms
} // namespace Mantid
