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

=====Setting the sample by specific values (all three must be specified)=====
 SetSampleMaterial(InputWorkspace='IRS26173',AttenuationXSection=2.56,ScatteringXSection=11.62,SampleNumberDensity=0.0849106)

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
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/Atom.h"
#include "MantidKernel/NeutronAtom.h"
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
        new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::InOut),
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
    declareProperty("UnitCellVolume", EMPTY_DBL(), mustBePositive,
        "Unit cell volume in Angstoms^3 needed for chemical formulas with more than 1 atom");
    declareProperty("ZParameter", EMPTY_DBL(), mustBePositive,
        "Number of formulas in the unit cell needed for chemical formulas with more than 1 atom");
    declareProperty("AttenuationXSection", EMPTY_DBL(), mustBePositive,
        "Optional:  This absorption cross-section for the sample material in barns will be used instead of calculated");
    declareProperty("ScatteringXSection", EMPTY_DBL(), mustBePositive,
        "Optional:  This scattering cross-section (coherent + incoherent) for the sample material in barns will be used instead of calculated");
    declareProperty("SampleNumberDensity", EMPTY_DBL(), mustBePositive,
        "Optional:  This number density of the sample in number per cubic angstrom will be used instead of calculated");
	
	// Perform Group Associations.
	std::string formulaGrp("By Formula or Atomic Number");
	setPropertyGroup("ChemicalFormula", formulaGrp);
	setPropertyGroup("AtomicNumber", formulaGrp);
	setPropertyGroup("MassNumber", formulaGrp);
	setPropertyGroup("UnitCellVolume", formulaGrp);
	setPropertyGroup("ZParameter", formulaGrp);

	std::string specificValuesGrp("Enter Specific Values");
	setPropertyGroup("AttenuationXSection", specificValuesGrp);
	setPropertyGroup("ScatteringXSection", specificValuesGrp);
	setPropertyGroup("SampleNumberDensity", specificValuesGrp);
  }

  /**
   * Execute the algorithm
   */
  void SetSampleMaterial::exec()
  {
    // Get the input workspace
    MatrixWorkspace_sptr workspace = getProperty("InputWorkspace");
    const std::string chemicalSymbol = getProperty("ChemicalFormula");
    const int z_number = getProperty("AtomicNumber");
    const int a_number = getProperty("MassNumber");
    double sigma_atten = getProperty("AttenuationXSection"); // in barns
    double sigma_s = getProperty("ScatteringXSection"); // in barns
    double rho = getProperty("SampleNumberDensity"); // in Angstroms-3
    double unitCellVolume = getProperty("UnitCellVolume"); // in Angstroms^3
    double zParameter = getProperty("ZParameter"); // in Angstroms^3

    // Use user variables if all three are given
    if (sigma_atten != EMPTY_DBL() && sigma_s != EMPTY_DBL() && rho != EMPTY_DBL())
    {
    	NeutronAtom *neutron = new NeutronAtom(static_cast<uint16_t>(z_number), static_cast<uint16_t>(a_number),
    			0.0, 0.0, sigma_s, 0.0, sigma_s, sigma_atten);
        Material *mat = new Material(chemicalSymbol, *neutron, rho);
        workspace->mutableSample().setMaterial(*mat);
        g_log.notice() << "Sample number density = "<< mat->numberDensity() << "\n";
        g_log.notice() << "Scattering X Section = " << mat->totalScatterXSection(NeutronAtom::ReferenceLambda) << "\n";
        g_log.notice() << "Attenuation X Section = " << mat->absorbXSection(NeutronAtom::ReferenceLambda)<< "\n";
    	return;
    }

    // Use chemical symbol if given by user
    try
    {
		Atom myAtom = getAtom(chemicalSymbol, static_cast<uint16_t>(a_number));
		Material *mat = new Material(chemicalSymbol, myAtom.neutron, myAtom.number_density);
		workspace->mutableSample().setMaterial(*mat);
		g_log.notice() << "Sample number density = "<< mat->numberDensity() << "\n";
		g_log.notice() << "Scattering X Section = " << mat->totalScatterXSection(NeutronAtom::ReferenceLambda) << "\n";
		g_log.notice() << "Attenuation X Section = " << mat->absorbXSection(NeutronAtom::ReferenceLambda)<< "\n";
    }
    catch (...)
    {
        // Use chemical formula if given by user
    	try
    	{
			Material::ChemicalFormula CF = Material::parseChemicalFormula(chemicalSymbol);
        	sigma_s = 0.0;
        	sigma_atten = 0.0;
        	for (size_t i=0; i<CF.atoms.size(); i++)
        	{
        		Atom myAtom = getAtom(CF.atoms[i], CF.aNumbers[i]);
        		Material *atom = new Material(CF.atoms[i], myAtom.neutron, myAtom.number_density);
        		g_log.notice() << myAtom << " sigma_s = "<< atom->totalScatterXSection(NeutronAtom::ReferenceLambda) << "\n";
        		g_log.notice() << myAtom << " sigma_atten = "<< atom->absorbXSection(NeutronAtom::ReferenceLambda) << "\n";
        		sigma_s +=  static_cast<double>(CF.numberAtoms[i]) * atom->totalScatterXSection(NeutronAtom::ReferenceLambda);
        		sigma_atten +=  static_cast<double>(CF.numberAtoms[i]) * atom->absorbXSection(NeutronAtom::ReferenceLambda);
        	}
			rho = zParameter / unitCellVolume;
			NeutronAtom *neutron = new NeutronAtom(static_cast<uint16_t>(z_number), static_cast<uint16_t>(a_number),
					0.0, 0.0, sigma_s, 0.0, sigma_s, sigma_atten);
    		Material *mat = new Material(chemicalSymbol, *neutron, rho);
	        g_log.notice() << "Sample number density = "<< mat->numberDensity() << "\n";
	        g_log.notice() << "Scattering X Section = " << mat->totalScatterXSection(NeutronAtom::ReferenceLambda) << "\n";
	        g_log.notice() << "Attenuation X Section = " << mat->absorbXSection(NeutronAtom::ReferenceLambda)<< "\n";
    		workspace->mutableSample().setMaterial(*mat);
    	}
        catch (...)
        {
			// Use atomic and mass number if chemical formula does not work
			try
			{
				Atom myAtom = getAtom(static_cast<uint16_t>(z_number), static_cast<uint16_t>(a_number));
				Material *mat = new Material(chemicalSymbol, myAtom.neutron, myAtom.number_density);
				workspace->mutableSample().setMaterial(*mat);
				g_log.notice() << "Sample number density = "<< mat->numberDensity() << "\n";
				g_log.notice() << "Scattering X Section = " << mat->totalScatterXSection(NeutronAtom::ReferenceLambda) << "\n";
				g_log.notice() << "Attenuation X Section = " << mat->absorbXSection(NeutronAtom::ReferenceLambda)<< "\n";
			}
			catch(std::invalid_argument&)
			{
				g_log.notice("ChemicalFormula or AtomicNumber was not found in table.");
				throw std::invalid_argument("ChemicalFormula or AtomicNumber was not found in table");
			}
        }
    }
    // Done!
    progress(1);
  }

}
}
