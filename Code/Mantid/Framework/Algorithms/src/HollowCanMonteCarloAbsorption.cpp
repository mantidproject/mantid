#include "MantidAlgorithms/HollowCanMonteCarloAbsorption.h"

#include "MantidAPI/SampleEnvironment.h"
#include "MantidAPI/WorkspaceValidators.h"

#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Objects/ShapeFactory.h"

#include "MantidKernel/Atom.h"
#include "MantidKernel/NeutronAtom.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/V3D.h"

#include <boost/shared_ptr.hpp>

#include <boost/format.hpp>

namespace Mantid
{
  namespace Algorithms
  {
    using namespace Mantid::API;
    using Mantid::Geometry::ObjComponent;
    using namespace Mantid::Kernel;

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(HollowCanMonteCarloAbsorption)


    //----------------------------------------------------------------------------------------------
    /** Constructor
    */
    HollowCanMonteCarloAbsorption::HollowCanMonteCarloAbsorption()
    {
    }
    
    //----------------------------------------------------------------------------------------------
    /** Destructor
    */
    HollowCanMonteCarloAbsorption::~HollowCanMonteCarloAbsorption()
    {
    }

    //----------------------------------------------------------------------------------------------


    /// Algorithm's name for identification. @see Algorithm::version
    const std::string HollowCanMonteCarloAbsorption::name() const
    {
      return "HollowCanMonteCarloAbsorption";
    }

    /// Algorithm's version for identification. @see Algorithm::version
    int HollowCanMonteCarloAbsorption::version() const
    {
      return 1;
    }

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string HollowCanMonteCarloAbsorption::category() const
    {
      return "CorrectionFunctions\\AbsorptionCorrections";
    }

     /// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
     const std::string HollowCanMonteCarloAbsorption::summary() const
     {
       return "Defines a hollow can + sachet for a sample holder and runs the Monte Carlo absorption algorithm.";
     }

    //----------------------------------------------------------------------------------------------
    /**
     * Initialize the algorithm's properties.
     */
    void HollowCanMonteCarloAbsorption::init()
    {
      // The input workspace must have an instrument and units of wavelength
      auto wsValidator = boost::make_shared<CompositeValidator>();
      wsValidator->add<WorkspaceUnitValidator>("Wavelength");
      wsValidator->add<InstrumentValidator>();
      declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,
                                              wsValidator),
                      "The input workspace in units of wavelength.");

      declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
                      "The name to use for the output workspace.");

      // -- sample properties --
      auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
      mustBePositive->setLower(0.0);
      declareProperty("SampleHeight", -1.0, mustBePositive,
                      "The height of the sample in centimetres");
      declareProperty("SampleThickness", -1.0, mustBePositive,
                      "The thickness of the sample in centimetres");

      auto nonEmptyString = boost::make_shared<MandatoryValidator<std::string>>();
      declareProperty("SampleChemicalFormula", "",
                      "Chemical composition of the sample material",
                      nonEmptyString);
      declareProperty("SampleNumberDensity", EMPTY_DBL(), mustBePositive,
                      "The number density of the sample in number of formulas per cubic angstrom");

      // -- can properties --
      declareProperty("CanOuterRadius", -1.0, mustBePositive,
                      "The outer radius of the can in centimetres");
      declareProperty("CanInnerRadius", -1.0, mustBePositive,
                      "The inner radius of the can in centimetres");
      declareProperty("CanSachetHeight", -1.0, mustBePositive,
                      "The height of the sachet in centimetres");
      declareProperty("CanSachetThickness", -1.0, mustBePositive,
                      "The thickness of the sachet in centimetres");
      declareProperty("CanMaterialFormula", "",
                      "Formula for the material that makes up the can. It is currently limited to a single atom type.",
                      nonEmptyString);

      // -- Monte Carlo properties --
      auto positiveInt = boost::make_shared<Kernel::BoundedValidator<int> >();
      positiveInt->setLower(1);
      declareProperty("NumberOfWavelengthPoints", EMPTY_INT(), positiveInt,
          "The number of wavelength points for which a simulation is atttempted (default: all points)");
      declareProperty("EventsPerPoint", 300, positiveInt,
          "The number of \"neutron\" events to generate per simulated point");
      declareProperty("SeedValue", 123456789, positiveInt,
          "Seed the random number generator with this value");

    }

    //----------------------------------------------------------------------------------------------
    /**
     * Execute the algorithm.
     */
    void HollowCanMonteCarloAbsorption::exec()
    {
      MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");

      attachEnvironment(inputWS);
      attachSample(inputWS);
    }

    //---------------------------------------------------------------------------------------------
    // Private members
    //---------------------------------------------------------------------------------------------

    /**
     * @param workspace The workspace where the environment should be attached
     */
    void HollowCanMonteCarloAbsorption::attachEnvironment(API::MatrixWorkspace_sptr &workspace)
    {
      auto envShape = createEnvironmentShape();
      auto envMaterial = createEnvironmentMaterial();

      SampleEnvironment * kit = new SampleEnvironment("HollowCylinder");
      kit->add(new ObjComponent("one", envShape, NULL, envMaterial));
      workspace->mutableSample().setEnvironment(kit);
    }

    /**
     * Create the XML that defines a hollow cylinder that encloses a sachet as a cuboid
     * @returns A shared_ptr to a new Geometry::Object that defines shape
     */
    boost::shared_ptr<Geometry::Object>
    HollowCanMonteCarloAbsorption::createEnvironmentShape() const
    {
      // User input
      const double outerRadiusCM = getProperty("CanOuterRadius");
      const double innerRadiusCM = getProperty("CanInnerRadius");
      const double sachetHeightCM = getProperty("CanSachetHeight");
      const double sachetThickCM = getProperty("CanSachetThickness");
      // Convert to metres
      const double outerRadiusMtr = outerRadiusCM/100.;
      const double innerRadiusMtr = innerRadiusCM/100.;
      const double sachetHeightMtr = sachetHeightCM/100.;
      const double sachetThickMtr = sachetThickCM/100.;

      // Cylinders oriented along Y, with origin at centre of bottom base
      const std::string outerCylID = std::string("outer-cyl");
      const std::string outerCyl = cylinderXML(outerCylID, V3D(), outerRadiusMtr, V3D(0.0, 1.0, 0.0), sachetHeightMtr);
      const std::string innerCylID = std::string("inner-cyl");
      const std::string innerCyl = cylinderXML(innerCylID, V3D(), innerRadiusMtr, V3D(0.0, 1.0, 0.0), sachetHeightMtr);

      // Sachet with origin in centre of bottom face
      // Format each face separately
      const std::string sachetID = std::string("sachet");
      const double halfSachetThickMtr = 0.5*sachetThickMtr;
      const std::string sachet = cuboidXML(sachetID,
                                           V3D(innerRadiusMtr, 0.0, -halfSachetThickMtr), //left front bottom
                                           V3D(innerRadiusMtr, 0.0, halfSachetThickMtr), // left back bottom top
                                           V3D(innerRadiusMtr, sachetHeightMtr, -halfSachetThickMtr), // left front top
                                           V3D(-innerRadiusMtr, 0.0, -halfSachetThickMtr)); // right front bottom
      // Combine shapes
      boost::format algebra("<algebra val=\"((%1% (# %2%)):%3%)\" />");
      algebra % outerCylID % innerCylID % sachetID;

      std::string fullXML = outerCyl + "\n" + innerCyl + "\n" + sachet + "\n" + algebra.str();
      if(g_log.is(Kernel::Logger::Priority::PRIO_DEBUG))  g_log.debug() << "Environment shape XML='" << fullXML << "'\n";
      Geometry::ShapeFactory shapeMaker;
      return shapeMaker.createShape(fullXML);
    }

    /**
     * Create a Kernel::Material object that models the environment's material. It is currently limited to a single atom
     * @return A share_ptr to a new Kernel::Material object
     */
    boost::shared_ptr<Kernel::Material> HollowCanMonteCarloAbsorption::createEnvironmentMaterial() const
    {
      const std::string chemicalSymbol = getPropertyValue("CanMaterialFormula");
      Material::ChemicalFormula formula;
      try
      {
        formula = Material::parseChemicalFormula(chemicalSymbol);
      }
      catch(std::runtime_error&)
      {
        throw std::invalid_argument("Unable to parse symbol string for can material");
      }
      // Only allow single atoms at present
      if(formula.atoms.size() > 1)
      {
        throw std::invalid_argument("Can material is currently restricted to a single atom.");
      }

      return boost::make_shared<Material>(chemicalSymbol, formula.atoms[0]->neutron, formula.atoms[0]->number_density);
    }

    /**
     * @param workspace The workspace where the environment should be attached
     */
    void HollowCanMonteCarloAbsorption::attachSample(MatrixWorkspace_sptr &workspace)
    {
//      auto sampleShape = createSampleShape();
//      auto & sample = workspace->mutableSample();
//      sample.setShape(*sampleShape);

      // Use SetSampleMaterial for the material
      runSetSampleMaterial(workspace);
    }

    /**
     * Create an object to model the sample shape. It will be a cuboid centred within the sachet created
     * by createEnvironmentShape
     * @return A new Geometry::Object that models the sample shape
     */
    boost::shared_ptr<Geometry::Object> HollowCanMonteCarloAbsorption::createSampleShape() const
    {
      const std::string xml;
      Geometry::ShapeFactory shapeMaker;
      return shapeMaker.createShape(xml);
    }

    /**
     * @param id String id of object
     * @param bottomCentre Point of centre of bottom base
     * @param radius Radius of cylinder
     * @param axis Cylinder will point along this axis
     * @param height The height of the cylinder
     * @return A string defining the XML
     */
    const std::string HollowCanMonteCarloAbsorption::cylinderXML(const std::string &id, const V3D &bottomCentre,
                                                                 const double radius, const V3D &axis, const double height) const
    {
      // The newline characters are not necessary for the XML but they make it easier to read for debugging
      static const char * CYL_TEMPLATE = \
        "<cylinder id=\"%1%\">\n"
        "<centre-of-bottom-base x=\"%2%\" y=\"%3%\" z=\"%4%\" />\n"
        " <axis x=\"%5%\" y=\"%6%\" z=\"%7%\" />\n"
        " <radius val=\"%8%\" />\n"
        " <height val=\"%9%\" />\n"
        "</cylinder>";

      boost::format xml(CYL_TEMPLATE);
      xml % id
          % bottomCentre.X() % bottomCentre.Y() % bottomCentre.Z()
          % axis.X() % axis.Y() % axis.Z()
          % radius % height;
      return xml.str();
    }

    /**
     * Create the XML required to construct a cuboid with the given points
     * @param id String id of the object
     * @param leftFrontBottom Position of left front bottom
     * @param leftBackBottom Position of back bottom
     * @param leftFrontTop Position of front front top
     * @param rightFrontBottom Position of right front bottom
     * @return A string defining the XML
     */
    const std::string HollowCanMonteCarloAbsorption::cuboidXML(const std::string & id,
                                                               const V3D &leftFrontBottom, const V3D &leftBackBottom,
                                                               const V3D &leftFrontTop, const V3D &rightFrontBottom) const
    {
      // The newline characters are not necessary for the XML but they make it easier to read for debugging
      static const char * CUBOID_TEMPLATE = \
          "<cuboid id=\"%1%\">\n"
          "<left-front-bottom-point x=\"%2%\" y=\"%3%\" z=\"%4%\" />\n"
          "<left-front-top-point x=\"%5%\" y=\"%6%\" z=\"%7%\" />\n"
          "<left-back-bottom-point x=\"%8%\" y=\"%9%\" z=\"%10%\" />\n"
          "<right-front-bottom-point x=\"%11%\" y=\"%12%\" z=\"%13%\" />\n"
          "</cuboid>";

      boost::format xml(CUBOID_TEMPLATE);
      xml % id
          % leftFrontBottom.X() % leftFrontBottom.Y() % leftFrontBottom.Z()
          % leftFrontTop.X() % leftFrontTop.Y() % leftFrontTop.Z()
          % leftBackBottom.X() % leftBackBottom.Y() % leftBackBottom.Z()
          % rightFrontBottom.X() % rightFrontBottom.Y() % rightFrontBottom.Z();
      return xml.str();
    }

    /**
     * @return Attaches a new Material object to the sample
     */
    void HollowCanMonteCarloAbsorption::runSetSampleMaterial(API::MatrixWorkspace_sptr & workspace)
    {
      auto alg = this->createChildAlgorithm("SetSampleMaterial", -1,-1, true);
      alg->setProperty("InputWorkspace", workspace);
      alg->setProperty("ChemicalFormula", getPropertyValue("SampleChemicalFormula"));
      alg->setProperty<double>("SampleNumberDensity", getProperty("SampleNumberDensity"));
      try
      {
        alg->executeAsChildAlg();
      }
      catch(std::exception & exc)
      {
        throw std::invalid_argument(std::string("Unable to set sample material: '") + exc.what() + "'");
      }
    }

  } // namespace Algorithms
} // namespace Mantid
