/*WIKI* 

Sets the neutrons information in the sample. You can either enter details about the chemical formula or atomic number, 
or you can provide specific values for the attenuation and scattering cross sections and the sample number density.  
If you decide to provide specific values you must give values for all three (attenuation and scattering cross sections and the sample number density), and any formula information will be ignored.
If you miss any of the three specific values then the other will be ignored.

Neutron scattering lengths and cross sections of the elements and their isotopes have been taken from [http://www.ncnr.nist.gov/resources/n-lengths/list.html].
 *WIKI*/
/*WIKI_USAGE* 
=====Setting the sample by simple formula=====
 SetSampleMaterial(InputWorkspace='IRS26173',ChemicalFormula='Fe')
 
=====Setting the sample by a more complex formula=====
 SetSampleMaterial(InputWorkspace='IRS26173',ChemicalFormula='Al2-O3', UnitCellVolume='253.54', ZParameter='6')

=====Setting the sample by specific values=====
 SetSampleMaterial(InputWorkspace='IRS26173',AtomicNumber=26,AttenuationXSection=2.56,ScatteringXSection=11.62,SampleNumberDensity=0.0849106)

=====Extracting the set values out by python=====
 sam = ws.sample()
 mat = sam.getMaterial()
 print mat.absorbXSection()
  1.3374
 print mat.cohScatterXSection()
  339.1712
 print mat.name()
  C2 H4
 print mat.totalScatterXSection()
  339.1712

 *WIKI_USAGE*/
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

  /// Sets documentation strings for this algorithm
  void SetSampleMaterial::initDocs()
  {
    this->setWikiSummary("Sets the neutrons information in the sample.");
    this->setOptionalMessage("Sets the neutrons information in the sample.");
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
    declareProperty("ChemicalFormula", "", "ChemicalFormula or AtomicNumber must be given. "
        "Enter a composition as a molecular formula of \n"
        "elements or isotopes.  For example, basic "
        "elements might be \"H\", \"Fe\" or \"Si\", etc. \n"
        "A molecular formula of elements might be "
        "\"H4-N2-C3\", which corresponds to a molecule \n"
        "with 4 Hydrogen atoms, 2 Nitrogen atoms and "
        "3 Carbon atoms.  Each element in a molecular \n"
        "formula is followed by the number of the atoms "
        "for that element, specified _without a hyphen_, \n"
        "because each element is separated from other "
        "elements using a hyphen.  The number of atoms \n"
        "can be integer or float, but must start with "
        "a digit, e.g. 0.6 is fine but .6 is not. \n"
        "Isotopes may also be included in a material "
        "composition, and can be specified alone \n"
        "(as in \"Li7\"), or in a molecular formula "
        "(as in \"(Li7)2-C-H4-N-Cl6\").  Note, however, \n"
        "that No Spaces or Hyphens are allowed in an "
        "isotope symbol specification.  Also Note \n"
        "that for isotopes specified in a molecular "
        "expression, the isotope must be enclosed \n"
        "by parenthesis, except for two special "
        "cases, D and T, which stand for H2 and H3, \n"
        "respectively.");
    declareProperty("AtomicNumber", 0, "ChemicalFormula or AtomicNumber must be given");
    declareProperty("MassNumber", 0, "Mass number if ion (default is 0)");
    auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
    mustBePositive->setLower(0.0);
    declareProperty("SampleNumberDensity", EMPTY_DBL(), mustBePositive,
        "Optional:  This number density of the sample in number of atoms per cubic angstrom will be used instead of calculated");
    declareProperty("ZParameter", EMPTY_DBL(), mustBePositive,
        "Number of formulas in the unit cell needed for chemical formulas with more than 1 atom");
    declareProperty("UnitCellVolume", EMPTY_DBL(), mustBePositive,
        "Unit cell volume in Angstoms^3 needed for chemical formulas with more than 1 atom");
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
    if (isEmpty(rho))
    {
      double unitCellVolume = getProperty("UnitCellVolume"); // in Angstroms^3
      double zParameter = getProperty("ZParameter"); // number of atoms

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
      g_log.notice() << "Found " << CF.atoms.size() << " atoms in \"" << chemicalSymbol << "\"\n";

      double numAtoms = 0.; // number of atoms in formula
      NeutronAtom neutron(0, 0., 0., 0., 0., 0., 0.); // starting thing for neutronic information
      for (size_t i=0; i<CF.atoms.size(); i++)
      {
        Atom myAtom = getAtom(CF.atoms[i], CF.aNumbers[i]);
        neutron = neutron + CF.numberAtoms[i] * myAtom.neutron;

        g_log.information() << myAtom << ": " << myAtom.neutron << "\n";
        numAtoms += static_cast<double>(CF.numberAtoms[i]);
      }
      // normalize the accumulated number by the number of atoms
      neutron = (1. / numAtoms) * neutron; // funny syntax b/c of operators in neutron atom

      fixNeutron(neutron, coh_xs, inc_xs, sigma_atten, sigma_s);

      // create the material
      mat.reset(new Material(chemicalSymbol, neutron, rho));
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

    // set the material on workspace
    expInfo->mutableSample().setMaterial(*mat);
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
    g_log.notice() << "Cross sections for wavelength = " << NeutronAtom::ReferenceLambda << "Angstroms\n"
                   << "    Coherent "   << mat->cohScatterXSection() << " barns\n"
                   << "    Incoherent " << mat->incohScatterXSection() << " barns\n"
                   << "    Total "      << mat->totalScatterXSection() << " barns\n"
                   << "    Absorption " << mat->absorbXSection() << " barns\n";
    setProperty("CoherentXSectionResult", mat->cohScatterXSection()); // in barns
    setProperty("IncoherentXSectionResult", mat->incohScatterXSection()); // in barns
    setProperty("TotalXSectionResult",mat->totalScatterXSection()); // in barns
    setProperty("AbsorptionXSectionResult",mat->absorbXSection()); // in barns
    setProperty("ReferenceWavelength",NeutronAtom::ReferenceLambda); // in Angstroms

    // Done!
    progress(1);
  }

}
}
