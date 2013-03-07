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
    declareProperty("ChemicalSymbol", "", "ChemicalSymbol or AtomicNumber must be given");
    declareProperty("AtomicNumber", EMPTY_INT(), "ChemicalSymbol or AtomicNumber must be given");
    declareProperty("MassNumber", 0, "Mass number of the atom to get (default is 0)");
    auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
    mustBePositive->setLower(0.0);
    declareProperty("AttenuationXSection", EMPTY_DBL(), mustBePositive,
      "Optional:  The ABSORPTION cross-section for the sample material in barns");
    declareProperty("ScatteringXSection", EMPTY_DBL(), mustBePositive,
      "Optional:  The scattering cross-section (coherent + incoherent) for the sample material in barns");
    declareProperty("SampleNumberDensity", EMPTY_DBL(), mustBePositive,
      "Optional:  The number density of the sample in number per cubic angstrom");
  }

  /**
   * Execute the algorithm
   */
  void SetSampleMaterial::exec()
  {
    // Get the input workspace
    MatrixWorkspace_sptr workspace = getProperty("InputWorkspace");
    const std::string chemicalSymbol = getProperty("ChemicalSymbol");
    std::vector<std::string> atoms;
    std::vector<uint16_t> numberAtoms, aNumbers;
    this->parseChemicalFormula(chemicalSymbol, atoms, numberAtoms, aNumbers);

    const int z_number = getProperty("AtomicNumber");
    const int a_number = getProperty("MassNumber");
    double sigma_atten = getProperty("AttenuationXSection"); // in barns
    double sigma_s = getProperty("ScatteringXSection"); // in barns
    double rho = getProperty("SampleNumberDensity"); // in Angstroms-3

    // Use user variables if all three are given
    if (sigma_atten != EMPTY_DBL() && sigma_s != EMPTY_DBL() && rho != EMPTY_DBL())
    {
    	NeutronAtom *neutron = new NeutronAtom(static_cast<uint16_t>(z_number), static_cast<uint16_t>(a_number),
    			0.0, 0.0, sigma_s, 0.0, sigma_s, sigma_atten);
        Material *mat = new Material(chemicalSymbol, *neutron, rho);
        workspace->mutableSample().setMaterial(*mat);
    	return;
    }

    // Use chemical formula if given by user
    try
    {
    	double mass = 0;
    	double density = 0;
    	sigma_s = 0;
    	sigma_atten = 0;
    	for (size_t i=0; i<atoms.size(); i++)
    	{
    		Atom myAtom = getAtom(atoms[i], aNumbers[i]);
    		mass += myAtom.mass * numberAtoms[i];
    	}
    	for (size_t i=0; i<atoms.size(); i++)
    	{
    	        NeutronAtom *neutron0 = new NeutronAtom(static_cast<uint16_t>(z_number), static_cast<uint16_t>(a_number),
    			0.0, 0.0, sigma_s, 0.0, sigma_s, sigma_atten);
    		Atom myAtom = getAtom(atoms[i], aNumbers[i]);
    		//rho = myAtom.number_density;
    		double mass0 = myAtom.mass * numberAtoms[i];
    		density += mass0/mass * myAtom.mass_density;
    		*neutron0 = myAtom.neutron;
    		Material *atom = new Material(atoms[i], *neutron0, rho);
    		sigma_s =  atom->totalScatterXSection(1.7982);
    		sigma_atten =  atom->absorbXSection(1.7982);
    		std::cout << atoms[i]<<"  "<< numberAtoms[i]<<"  "<<aNumbers[i]<<"  "<<mass0 <<"  "<<mass<<"  "<< myAtom.mass_density<<"  "<<density<<"  "<<sigma_s<<"  "<<sigma_atten<<"\n";
    	}
         	rho = density * PhysicalConstants::N_A * 1.e-24/ mass;
    	        NeutronAtom *neutron = new NeutronAtom(static_cast<uint16_t>(z_number), static_cast<uint16_t>(a_number),
    			0.0, 0.0, sigma_s, 0.0, sigma_s, sigma_atten);
		Material *mat = new Material(chemicalSymbol, *neutron, rho);
		std::cout << atoms.size()<<"  "<<rho<<"  "<<mat->totalScatterXSection(1.7982)<<"  "<<mat->absorbXSection(1.7982)<<"\n";
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
    	}
    	catch(std::invalid_argument&)
    	{
    		g_log.information("ChemicalSymbol or AtomicNumber was not found in table.");
    		throw std::invalid_argument("ChemicalSymbol or AtomicNumber was not found in table");
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
