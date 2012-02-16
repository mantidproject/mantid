/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidDataHandling/LoadLiveData.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace DataHandling
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(LoadLiveData)
  

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LoadLiveData::LoadLiveData()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LoadLiveData::~LoadLiveData()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string LoadLiveData::name() const { return "LoadLiveData";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int LoadLiveData::version() const { return 1;};
  
  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void LoadLiveData::initDocs()
  {
    this->setWikiSummary("Load a chunk of live data. You should call StartLiveData, and not this algorithm directly.");
    this->setOptionalMessage("Load a chunk of live data. You should call StartLiveData, and not this algorithm directly.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void LoadLiveData::init()
  {
    this->initProps();
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void LoadLiveData::exec()
  {

  }



} // namespace Mantid
} // namespace DataHandling
