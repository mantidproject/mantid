/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidMDAlgorithms/SaveZODS.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SaveZODS)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SaveZODS::SaveZODS()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SaveZODS::~SaveZODS()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string SaveZODS::name() const { return "SaveZODS";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int SaveZODS::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string SaveZODS::category() const { return "MDAlgorithms";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void SaveZODS::initDocs()
  {
    this->setWikiSummary("Save a [[MDHistoWorkspace]] to a HDF5 format for use with the ZODS analysis software.");
    this->setOptionalMessage("Save a MDHistoWorkspace to a HDF5 format for use with the ZODS analysis software.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SaveZODS::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "An input workspace.");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SaveZODS::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace Mantid
} // namespace MDAlgorithms
