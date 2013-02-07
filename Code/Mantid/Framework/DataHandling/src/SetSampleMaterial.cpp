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
#include "MantidKernel/NeutronAtom.h"
#include "MantidGeometry/Objects/Material.h"

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
    declareProperty("materialName", "", "Name of Material");
    declareProperty("numberDensity", EMPTY_DBL(), "Density in A^-3");
    declareProperty("zNumber", EMPTY_INT(), "Atomic number of the atom to get");
    declareProperty("aNumber", 0, "Mass number of the atom to get");
  }

  /**
   * Execute the algorithm
   */
  void SetSampleMaterial::exec()
  {
    // Get the input workspace
    MatrixWorkspace_sptr workspace = getProperty("InputWorkspace");
    double numberDensity = getProperty("numberDensity");
    std::string materialName = getProperty("materialName");
    int z_number = getProperty("zNumber");
    int a_number = getProperty("aNumber");

    Material *mat = new Material(materialName, PhysicalConstants::getNeutronAtom(z_number,a_number), numberDensity);
    workspace->mutableSample().setMaterial(*mat);
    // Done!
    progress(1);
  }

}
}
