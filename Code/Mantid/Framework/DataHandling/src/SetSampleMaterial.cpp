/*WIKI* 

Sets the neutrons information in the sample.


*WIKI*/
//--------------------------------
// Includes
//--------------------------------
#include "MantidDataHandling/SetSampleMaterial.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/Atom.h"
#include "MantidKernel/NeutronAtom.h"
#include "MantidGeometry/Objects/Material.h"
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
  using namespace Geometry;

  /**
   * Initialize the algorithm
   */
  void SetSampleMaterial::init()
  {
    using namespace Mantid::Kernel;
    declareProperty(
        new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input),
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
      "Unit cell volumne in Angstoms^3 needed for chemical formulas with more than 1 atom");
    declareProperty("ZParameter", EMPTY_DBL(), mustBePositive,
      "Number of formulas in the unit cell needed for chemical formulas with more than 1 atom");
    declareProperty("AttenuationXSection", EMPTY_DBL(), mustBePositive,
      "Optional:  This absorption cross-section for the sample material in barns will be used instead of calculated");
    declareProperty("ScatteringXSection", EMPTY_DBL(), mustBePositive,
      "Optional:  This scattering cross-section (coherent + incoherent) for the sample material in barns will be used instead of calculated");
    declareProperty("SampleNumberDensity", EMPTY_DBL(), mustBePositive,
      "Optional:  This number density of the sample in number per cubic angstrom will be used instead of calculated");
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
        g_log.notice() << "Scattering X Section = " << mat->totalScatterXSection(1.7982) << "\n";
        g_log.notice() << "Attenuation X Section = " << mat->absorbXSection(1.7982)<< "\n";
    	return;
    }

    // Use chemical symbol if given by user
    try
    {
		Atom myAtom = getAtom(chemicalSymbol, static_cast<uint16_t>(a_number));
		Material *mat = new Material(chemicalSymbol, myAtom.neutron, myAtom.number_density);
		workspace->mutableSample().setMaterial(*mat);
		g_log.notice() << "Sample number density = "<< mat->numberDensity() << "\n";
		g_log.notice() << "Scattering X Section = " << mat->totalScatterXSection(1.7982) << "\n";
		g_log.notice() << "Attenuation X Section = " << mat->absorbXSection(1.7982)<< "\n";
    }
    catch (...)
    {
        // Use chemical formula if given by user
    	try
    	{
			std::vector<std::string> atoms;
			std::vector<uint16_t> numberAtoms, aNumbers;
			this->parseChemicalFormula(chemicalSymbol, atoms, numberAtoms, aNumbers);
        	sigma_s = 0.0;
        	sigma_atten = 0.0;
        	for (size_t i=0; i<atoms.size(); i++)
        	{
        		Atom myAtom = getAtom(atoms[i], aNumbers[i]);
        		Material *atom = new Material(atoms[i], myAtom.neutron, myAtom.number_density);
        		g_log.notice() << myAtom << " sigma_s = "<< atom->totalScatterXSection(1.7982) << "\n";
        		g_log.notice() << myAtom << " sigma_atten = "<< atom->absorbXSection(1.7982) << "\n";
        		sigma_s +=  static_cast<double>(numberAtoms[i]) * atom->totalScatterXSection(1.7982);
        		sigma_atten +=  static_cast<double>(numberAtoms[i]) * atom->absorbXSection(1.7982);
        	}
			rho = zParameter / unitCellVolume;
			NeutronAtom *neutron = new NeutronAtom(static_cast<uint16_t>(z_number), static_cast<uint16_t>(a_number),
					0.0, 0.0, sigma_s, 0.0, sigma_s, sigma_atten);
    		Material *mat = new Material(chemicalSymbol, *neutron, rho);
	        g_log.notice() << "Sample number density = "<< mat->numberDensity() << "\n";
	        g_log.notice() << "Scattering X Section = " << mat->totalScatterXSection(1.7982) << "\n";
	        g_log.notice() << "Attenuation X Section = " << mat->absorbXSection(1.7982)<< "\n";
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
				g_log.notice() << "Scattering X Section = " << mat->totalScatterXSection(1.7982) << "\n";
				g_log.notice() << "Attenuation X Section = " << mat->absorbXSection(1.7982)<< "\n";
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
  void SetSampleMaterial::parseChemicalFormula(const std::string chemicalSymbol, std::vector<std::string>& atoms,
		  std::vector<uint16_t>& numberAtoms, std::vector<uint16_t>& aNumbers)
  {
	  const char *s;
	  s = chemicalSymbol.c_str();
	  size_t i = 0;
	  size_t ia = 0;
	  size_t numberParen = 0;
	  size_t sizeParen = 0;
	  bool isotope = false;
	  while (i < chemicalSymbol.length())
	  {
		if (s[i] >= 'A' && s[i]<='Z')
		{
			std::string buf(s+i, s+i+1);
			atoms.push_back(buf);
			numberAtoms.push_back(0);
			aNumbers.push_back(0);
			ia ++;
		}
		else if (s[i] >= 'a' && s[i]<='z')
		{
			std::string buf(s+i, s+i+1);
			atoms[ia-1].append(buf);
		}
		else if (s[i] >= '0' && s[i]<='9')
		{
			if (isotope)
			{
				size_t ilast = i;
				// Number of digits in aNumber
				if (aNumbers[ia-1] != 0) ilast -= (int) std::log10 ((double) aNumbers[ia-1]) + 1;
				std::string buf(s+ilast, s+i+1);
				aNumbers[ia-1] = static_cast<uint16_t>(std::atoi(buf.c_str()));
			}
			else
			{
				size_t ilast = i;
				// Number of digits in aNumber
				if (numberAtoms[ia-1] != 0) ilast -= (int) std::log10 ((double) numberAtoms[ia-1]) + 1;
				std::string buf(s+ilast, s+i+1);
				numberAtoms[ia-1] = static_cast<uint16_t>(std::atoi(buf.c_str()));
			}

		}
		else if (s[i] == '(' || s[i] ==')')
		{
			isotope = !isotope;
			if (s[i] == '(')
			{
                                // next atom
                                sizeParen = 0;
				numberParen = ia + 1;
			}
			else
			{
                                sizeParen = ia - numberParen + 1;
				if (ia > numberParen)for (size_t i0 = numberParen - 1; i0 < ia; i0++)
				{
					  // if more than one atom in parenthesis, it is compound
					  numberAtoms[i0] = aNumbers[i0];
					  aNumbers[i0] = 0;
				}
			}
		}
		else
		{
		}
		i++;
	  }
		if (ia == 1 && s[0] != '(')
		{
			  // isotopes in molecular expressions must have parentheses
			  // single isotopes can omit parentheses
			  aNumbers[0] = numberAtoms[0];
			  numberAtoms[0] = 1;
		}
		for (size_t i0=0; i0<ia; i0++)
		{
                        if (numberAtoms[i0] == 0)numberAtoms[i0] = 1;
			if (atoms[i0].compare("D") == 0)
			{
				atoms[i0] = "H";
				aNumbers[i0] = 2;
			}
			else if (atoms[i0].compare("T") == 0)
			{
				atoms[i0] = "H";
				aNumbers[i0] = 3;
			}
		}
  }
}
}
