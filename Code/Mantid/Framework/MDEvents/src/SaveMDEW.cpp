#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/SaveMDEW.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SaveMDEW)


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SaveMDEW::SaveMDEW()
  {
    // TODO Auto-generated constructor stub
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SaveMDEW::~SaveMDEW()
  {
    // TODO Auto-generated destructor stub
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void SaveMDEW::initDocs()
  {
    this->setWikiSummary("Save a MDEventWorkspace to a .nxs file.");
    this->setOptionalMessage("Save a MDEventWorkspace to a .nxs file.");
    this->setWikiDescription("Save a MDEventWorkspace to a .nxs file.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SaveMDEW::init()
  {
    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace","",Direction::Input), "An input MDEventWorkspace.");

    std::vector<std::string> exts;
    exts.push_back(".nxs");
    declareProperty(new FileProperty("Filename", "", FileProperty::Save, exts),
        "The name of the Nexus file to write, as a full or relative path");
  }

  //----------------------------------------------------------------------------------------------
  /** Templated method to do the work
   *
   * @param ws :: MDEventWorkspace of the given type
   */
  template<typename MDE, size_t nd>
  void SaveMDEW::doSave(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    std::string filename = getPropertyValue("Filename");
    //ws->getBox();
    UNUSED_ARG(ws);
  }


  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SaveMDEW::exec()
  {
    IMDEventWorkspace_sptr ws = getProperty("InputWorkspace");

    // Wrapper to cast to MDEventWorkspace then call the function
    CALL_MDEVENT_FUNCTION(this->doSave, ws);
  }



} // namespace Mantid
} // namespace MDEvents

