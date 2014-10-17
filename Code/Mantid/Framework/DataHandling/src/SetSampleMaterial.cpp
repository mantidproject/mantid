//--------------------------------
// Includes
//--------------------------------
#include "MantidDataHandling/SetSampleMaterial.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/Atom.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/PhysicalConstants.h"

using namespace Mantid::PhysicalConstants;

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(SetSampleMaterial)

    SetSampleMaterial::SetSampleMaterial() : Mantid::API::Algorithm() {}
    SetSampleMaterial::~SetSampleMaterial() {}
    const std::string SetSampleMaterial::name() const
    {
      return "SetSampleMaterial";
    }

    int SetSampleMaterial::version() const
    {
      return (1);
    }

    const std::string SetSampleMaterial::category() const
    {
      return "Sample;DataHandling";
    }

    using namespace Mantid::DataHandling;
    using namespace Mantid::API;
    using namespace Kernel;

    /**
    * Initialize the algorithm
    */
    void SetSampleMaterial::init()
    {
      using namespace Mantid::Kernel;
      declareProperty(
        new WorkspaceProperty<Workspace>("InputWorkspace","",Direction::InOut),
        "The workspace with which to associate the sample ");
      declareProperty("ChemicalFormula", "", "ChemicalFormula or AtomicNumber must be given.");
      declareProperty("AtomicNumber", 0, "ChemicalFormula or AtomicNumber must be given");
      declareProperty("MassNumber", 0, "Mass number if ion (default is 0)");
      auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
      mustBePositive->setLower(0.0);
      declareProperty("SampleNumberDensity", EMPTY_DBL(), mustBePositive,
        "Optional:  This number density of the sample in number of formulas per cubic angstrom will be used instead of calculated");
      declareProperty("ZParameter", EMPTY_DBL(), mustBePositive,
        "Number of atoms in the unit cell");
      declareProperty("UnitCellVolume", EMPTY_DBL(), mustBePositive,
        "Unit cell volume in Angstoms^3. Will be calculated from the OrientedLattice if not supplied.");
      declareProperty("CoherentXSection", EMPTY_DBL(), mustBePositive,
        "Optional:  This coherent cross-section for the sample material in barns will be used instead of tabulated");
      declareProperty("IncoherentXSection", EMPTY_DBL(), mustBePositive,
        "Optional:  This incoherent cross-section for the sample material in barns will be used instead of tabulated");
      declareProperty("AttenuationXSection", EMPTY_DBL(), mustBePositive,
        "Optional:  This absorption cross-section for the sample material in barns will be used instead of tabulated");
      declareProperty("ScatteringXSection", EMPTY_DBL(), mustBePositive,
        "Optional:  This total scattering cross-section (coherent + incoherent) for the sample material in barns will be used instead of tabulated");

      // Perform Group Associations.
      std::string formulaGrp("By Formula or Atomic Number");
      setPropertyGroup("ChemicalFormula", formulaGrp);
      setPropertyGroup("AtomicNumber", formulaGrp);
      setPropertyGroup("MassNumber", formulaGrp);

      std::string densityGrp("Sample Density");
      setPropertyGroup("SampleNumberDensity", densityGrp);
      setPropertyGroup("ZParameter", densityGrp);
      setPropertyGroup("UnitCellVolume", densityGrp);

      std::string specificValuesGrp("Override Cross Section Values");
      setPropertyGroup("CoherentXSection", specificValuesGrp);
      setPropertyGroup("IncoherentXSection", specificValuesGrp);
      setPropertyGroup("AttenuationXSection", specificValuesGrp);
      setPropertyGroup("ScatteringXSection", specificValuesGrp);

      // Extra property settings
      setPropertySettings("AtomicNumber", new Kernel::EnabledWhenProperty("ChemicalFormula", Kernel::IS_DEFAULT));
      setPropertySettings("MassNumber", new Kernel::EnabledWhenProperty("ChemicalFormula", Kernel::IS_DEFAULT));

      setPropertySettings("UnitCellVolume", new Kernel::EnabledWhenProperty("SampleNumberDensity", Kernel::IS_DEFAULT));
      setPropertySettings("ZParameter", new Kernel::EnabledWhenProperty("SampleNumberDensity", Kernel::IS_DEFAULT));

      //output properties
      declareProperty("SampleNumberDensityResult", EMPTY_DBL(), "The provided or calculated sample number density in atoms/Angstrom^3", Direction::Output); 
      declareProperty("ReferenceWavelength", EMPTY_DBL(), "The reference wavelength in Angstroms", Direction::Output);
      declareProperty("TotalXSectionResult", EMPTY_DBL(), "The provided or calculated total cross-section for the sample material in barns.", Direction::Output);
      declareProperty("IncoherentXSectionResult", EMPTY_DBL(), "The provided or calculated incoherent cross-section for the sample material in barns.", Direction::Output);
      declareProperty("CoherentXSectionResult", EMPTY_DBL(), "The provided or calculated coherent cross-section for the sample material in barns.", Direction::Output);
      declareProperty("AbsorptionXSectionResult", EMPTY_DBL(),"The provided or calculated Absorption cross-section for the sample material in barns.", Direction::Output);

    }

    std::map<std::string, std::string> SetSampleMaterial::validateInputs()
    {
      std::map<std::string, std::string> result;
      const std::string chemicalSymbol = getProperty("ChemicalFormula");
      const int z_number = getProperty("AtomicNumber");
      const int a_number = getProperty("MassNumber");
      if (chemicalSymbol.empty())
      {
        if (z_number <= 0)
        {
          result["ChemicalFormula"] = "Need to specify the material";

        }
      }
      else
      {
        if (z_number > 0)
          result["AtomicNumber"] = "Cannot specify both ChemicalFormula and AtomicNumber";
      }

      if (a_number > 0 && z_number <= 0)
        result["AtomicNumber"] = "Specified MassNumber without AtomicNumber";

      return result;
    }

    /**
    * Add the cross sections to the neutron atom if they are not-empty
    * numbers. All values are in barns.
    *
    * @param neutron The neutron to update
    * @param coh_xs Coherent cross section
    * @param inc_xs Incoherent cross section
    * @param abs_xs Absorption cross section
    * @param tot_xs Total scattering cross section
    */
    void SetSampleMaterial::fixNeutron(NeutronAtom &neutron,
      double coh_xs, double inc_xs,
      double abs_xs, double tot_xs)
    {
      if (!isEmpty(coh_xs))
        neutron.coh_scatt_xs = coh_xs;
      if (!isEmpty(inc_xs))
        neutron.inc_scatt_xs = inc_xs;
      if (!isEmpty(abs_xs))
        neutron.abs_scatt_xs = abs_xs;
      if (!isEmpty(tot_xs))
        neutron.tot_scatt_xs = tot_xs;
    }

    /**
    * Execute the algorithm
    */
    void SetSampleMaterial::exec()
    {
      // Get the input workspace
      Workspace_sptr workspace = getProperty("InputWorkspace");
      // an ExperimentInfo object has a sample
      ExperimentInfo_sptr expInfo = boost::dynamic_pointer_cast<ExperimentInfo>(workspace);
      if (!bool(expInfo))
      {
        throw std::runtime_error("InputWorkspace does not have a sample object");
      }

      // determine the sample number density
      double rho = getProperty("SampleNumberDensity"); // in Angstroms-3
      double zParameter = getProperty("ZParameter"); // number of atoms
      if (isEmpty(rho))
      {
        double unitCellVolume = getProperty("UnitCellVolume"); // in Angstroms^3

        // get the unit cell volume from the workspace if it isn't set
        if (isEmpty(unitCellVolume) && expInfo->sample().hasOrientedLattice())
        {
          unitCellVolume = expInfo->sample().getOrientedLattice().volume();
          g_log.notice() << "found unit cell volume " << unitCellVolume << " Angstrom^-3\n";
        }
        // density is just number of atoms in the unit cell
        // ...but only calculate it if you have both numbers
        if ((!isEmpty(zParameter)) && (!isEmpty(unitCellVolume)))
          rho = zParameter / unitCellVolume;
      }

      // get the scattering information - this will override table values
      double coh_xs = getProperty("CoherentXSection"); // in barns
      double inc_xs = getProperty("IncoherentXSection"); // in barns
      double sigma_atten = getProperty("AttenuationXSection"); // in barns
      double sigma_s = getProperty("ScatteringXSection"); // in barns

      // determine the material
      const std::string chemicalSymbol = getProperty("ChemicalFormula");
      const int z_number = getProperty("AtomicNumber");
      const int a_number = getProperty("MassNumber");

      boost::scoped_ptr<Material> mat;
      if (!chemicalSymbol.empty())
      {
        // Use chemical formula if given by user
        Material::ChemicalFormula CF = Material::parseChemicalFormula(chemicalSymbol);
        g_log.notice() << "Found " << CF.atoms.size() << " types of atoms in \""
          << chemicalSymbol << "\"\n";


        NeutronAtom neutron(0, 0., 0., 0., 0., 0., 0.); // starting thing for neutronic information
        if (CF.atoms.size() == 1 && isEmpty(zParameter) && isEmpty(rho))
        {
          mat.reset(new Material(chemicalSymbol, CF.atoms[0]->neutron, CF.atoms[0]->number_density));
        }
        else
        {
          double numAtoms = 0.; // number of atoms in formula
          for (size_t i=0; i<CF.atoms.size(); i++)
          {
            neutron = neutron + CF.numberAtoms[i] * CF.atoms[i]->neutron;

            g_log.information() << CF.atoms[i] << ": " << CF.atoms[i]->neutron << "\n";
            numAtoms += static_cast<double>(CF.numberAtoms[i]);
          }
          // normalize the accumulated number by the number of atoms
          neutron = (1. / numAtoms) * neutron; // funny syntax b/c of operators in neutron atom

          fixNeutron(neutron, coh_xs, inc_xs, sigma_atten, sigma_s);

          // create the material
          mat.reset(new Material(chemicalSymbol, neutron, rho));
        }
      }
      else
      {
        // try using the atomic number
        Atom atom = getAtom(static_cast<uint16_t>(z_number), static_cast<uint16_t>(a_number));
        NeutronAtom neutron = atom.neutron;
        fixNeutron(neutron, coh_xs, inc_xs, sigma_atten, sigma_s);

        // create the material
        mat.reset(new Material(chemicalSymbol, neutron, rho));
      }

      // set the material but leave the geometry unchanged
      auto shapeObject = expInfo->sample().getShape(); //copy 
      shapeObject.setMaterial(*mat);
      expInfo->mutableSample().setShape(shapeObject);
      g_log.notice() << "Sample number density ";
      if (isEmpty(mat->numberDensity()))
      {
        g_log.notice() << "was not specified\n";
      }
      else
      {
        g_log.notice() << "= " << mat->numberDensity() << " atoms/Angstrom^3\n";
        setProperty("SampleNumberDensityResult", mat->numberDensity()); // in atoms/Angstrom^3
      }
      g_log.notice() << "Cross sections for wavelength = " << NeutronAtom::ReferenceLambda << " Angstroms\n"
        << "    Coherent "   << mat->cohScatterXSection() << " barns\n"
        << "    Incoherent " << mat->incohScatterXSection() << " barns\n"
        << "    Total "      << mat->totalScatterXSection() << " barns\n"
        << "    Absorption " << mat->absorbXSection() << " barns\n";
      setProperty("CoherentXSectionResult", mat->cohScatterXSection()); // in barns
      setProperty("IncoherentXSectionResult", mat->incohScatterXSection()); // in barns
      setProperty("TotalXSectionResult",mat->totalScatterXSection()); // in barns
      setProperty("AbsorptionXSectionResult",mat->absorbXSection()); // in barns
      setProperty("ReferenceWavelength",NeutronAtom::ReferenceLambda); // in Angstroms

      if (isEmpty(rho))
      {
          g_log.notice("Unknown value for number density");
      }
      else
      {
          double smu =  mat->totalScatterXSection(NeutronAtom::ReferenceLambda) * rho;
          double amu = mat->absorbXSection(NeutronAtom::ReferenceLambda) * rho;
          g_log.notice() << "Anvred LinearScatteringCoef = " << smu << " 1/cm\n"
                         << "Anvred LinearAbsorptionCoef = "   << amu << " 1/cm\n";
      }
      // Done!
      progress(1);
    }

  }
}
