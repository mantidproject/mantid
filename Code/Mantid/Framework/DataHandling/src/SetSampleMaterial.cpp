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
  }

  /**
   * Execute the algorithm
   */
  void SetSampleMaterial::exec()
  {
    // Get the input workspace
    MatrixWorkspace_sptr workspace = getProperty("InputWorkspace");
    const std::string chemicalSymbol = getProperty("ChemicalSymbol");
    const int z_number = getProperty("AtomicNumber");
    const int a_number = getProperty("MassNumber");

    try
    {
      Atom myAtom = getAtom(chemicalSymbol, static_cast<uint16_t>(a_number));
      Material *mat = new Material(chemicalSymbol, myAtom.neutron, myAtom.number_density);
      workspace->mutableSample().setMaterial(*mat);
    }
    catch (...)
    {
    	try
    	{
			Atom myAtom = getAtom(static_cast<uint16_t>(z_number), static_cast<uint16_t>(a_number));
			Material *mat = new Material(chemicalSymbol, myAtom.neutron, myAtom.number_density);
			workspace->mutableSample().setMaterial(*mat);
    	}
    	catch(std::invalid_argument&)
    	{
    		g_log.information("ChemicalSymbol or AtomicNumber must be given.");
    		throw std::invalid_argument("ChemicalSymbol or AtomicNumber must be given");
    	}
    }
    // Done!
    progress(1);
  }

}
}
