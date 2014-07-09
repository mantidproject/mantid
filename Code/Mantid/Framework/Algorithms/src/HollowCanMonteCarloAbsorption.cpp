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

#include <boost/shared_ptr.hpp>

#include <Poco/Format.h>

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
                      "The input workspace in units of wavelength");

      declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
                      "The name to use for the output workspace.");

      // -- sample properties --
      auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
      mustBePositive->setLower(0.0);
      declareProperty("SampleHeight", -1.0, mustBePositive,
        "The height of the sample in centimetres");
      declareProperty("SampleThickness", -1.0, mustBePositive,
        "The thickness of the sample in centimetres");

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
                      boost::make_shared<MandatoryValidator<std::string>>());

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
      addEnvironment(inputWS);
    }

    //---------------------------------------------------------------------------------------------
    // Private members
    //---------------------------------------------------------------------------------------------

    /**
     * @param workspace The workspace where the environment should be attached
     */
    void HollowCanMonteCarloAbsorption::addEnvironment(const API::MatrixWorkspace_sptr &workspace)
    {
      const double outerRadiusCM = getProperty("CanOuterRadius");
      const double innerRadiusCM = getProperty("CanInnerRadius");
      const double sachetHeightCM = getProperty("CanSachetHeight");
      const double sachetThickCM = getProperty("CanSachetThickness");

      auto envShape = createEnvironmentShape(outerRadiusCM, innerRadiusCM, sachetHeightCM, sachetThickCM);
      auto envMaterial = createEnvironmentMaterial(getPropertyValue("CanMaterialFormula"));

      SampleEnvironment * kit = new SampleEnvironment("HollowCylinder");
      kit->add(new ObjComponent("one", envShape, NULL, envMaterial));
      workspace->mutableSample().setEnvironment(kit);
    }

    /**
     * Create the XML that defines a hollow cylinder that encloses a sachet
     * @param outerRadiusCM Radius of the outer edge of the cylinder in cm
     * @param innerRadiusCM Radius of the inner edge of the cylinder in cm
     * @param sachetHeightCM Height of the sachet holding the sample in cm
     * @param sachetThickCM Thickness of the sachet holding the sample in cm
     * @returns A shared_ptr to a new Geometry::Object that defines shape
     */
    boost::shared_ptr<Geometry::Object>
    HollowCanMonteCarloAbsorption::createEnvironmentShape(const double outerRadiusCM, const double innerRadiusCM,
                                                          const double sachetHeightCM, const double sachetThickCM) const
    {
      // The newline characters are not necessary for the XML but they make it easier to read for debugging
      static const char * CYL_TEMPLATE = \
        "<cylinder id=\"%s\">\n"
        "<centre-of-bottom-base r=\"0.0\" t=\"0.0\" p=\"0.0\" />\n"
        " <axis x=\"0.0\" y=\"1.0\" z=\"0.0\" />\n"
        "  <radius val=\"%f\" />\n"
        "  <height val=\"%f\" />\n"
        "</cylinder>";

      static const char * CUBOID_TEMPLATE = \
          "<cuboid id=\"%s\">\n"
          "<left-front-bottom-point x=\"%f\" y=\"%f\" z=\"%f\" />\n"
          "<left-front-top-point x=\"%f\" y=\"%f\" z=\"%f\" />\n"
          "<left-back-bottom-point x=\"%f\" y=\"%f\" z=\"%f\" />\n"
          "<right-front-bottom-point x=\"%f\" y=\"%f\" z=\"%f\" />\n"
          "</cuboid>";

      // Convert to metres
      const double outerRadiusMtr = outerRadiusCM/100.;
      const double innerRadiusMtr = innerRadiusCM/100.;
      const double sachetHeightMtr = sachetHeightCM/100.;
      const double sachetThickMtr = sachetThickCM/100.;

      // Cylinders oriented along Y, with origin at centre of bottom base
      const std::string outerCylID = std::string("outer-cyl");
      const std::string outerCyl = Poco::format(CYL_TEMPLATE, outerCylID, outerRadiusMtr, sachetHeightMtr);
      const std::string innerCylID = std::string("inner-cyl");
      const std::string innerCyl = Poco::format(CYL_TEMPLATE, innerCylID, innerRadiusMtr, sachetHeightMtr);

      // Sachet with origin in centre of bottom face
      // Format each face separately
      const std::string sachetID = std::string("sachet");
      const double halfSachetThickMtr = 0.5*sachetThickMtr;
      std::string sachet = Poco::format(CUBOID_TEMPLATE, sachetID, innerRadiusMtr, 0.0, -halfSachetThickMtr);
      sachet = Poco::format(sachet, innerRadiusMtr, sachetHeightMtr, -halfSachetThickMtr);
      sachet = Poco::format(sachet, innerRadiusMtr, sachetHeightMtr, halfSachetThickMtr);
      sachet = Poco::format(sachet, -innerRadiusMtr, 0.0, -halfSachetThickMtr);

      // Combine shapes
      std::string algebra = Poco::format("<algebra val=\"((%s (# %s)):%s)\" />",outerCylID, innerCylID, sachetID);
      std::string fullXML = outerCyl + "\n" + innerCyl + "\n" + sachet + "\n" + algebra;

      if(g_log.is(Kernel::Logger::Priority::PRIO_DEBUG))  g_log.debug() << "Environment shape XML='" << fullXML << "'\n";

      Geometry::ShapeFactory shapeMaker;
      return shapeMaker.createShape(fullXML);
    }

    /**
     * Create a Kernel::Material object that models the environment's material. It is currently limited to a single atom
     * @param chemicalSymbol A string giving the chemical symbol of the atom
     * @return A share_ptr to a new Kernel::Material object
     */
    boost::shared_ptr<Kernel::Material> HollowCanMonteCarloAbsorption::createEnvironmentMaterial(const std::string &chemicalSymbol) const
    {
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

  } // namespace Algorithms
} // namespace Mantid
