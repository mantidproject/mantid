#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/CloneMDEventWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
#include <Poco/File.h>
#include <Poco/Path.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(CloneMDEventWorkspace)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  CloneMDEventWorkspace::CloneMDEventWorkspace()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  CloneMDEventWorkspace::~CloneMDEventWorkspace()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void CloneMDEventWorkspace::initDocs()
  {
    this->setWikiSummary("Clones (copies) an existing MDEventWorkspace into a new one.");
    this->setOptionalMessage("Clones (copies) an existing [[MDEventWorkspace]] into a new one.");
    this->setWikiDescription(""
        "This algorithm will clones an existing MDEventWorkspace into a new one."
        "\n\n"
        "If the InputWorkspace is a file-backed MDEventWorkspace, then the algorithm will copy"
        " the original file into a new one with the suffix '_clone' added to its filename, in the same directory.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void CloneMDEventWorkspace::init()
  {
    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace","",Direction::Input),
        "An input MDEventWorkspace.");
    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("OutputWorkspace","",Direction::Output),
        "Name of the output MDEventWorkspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Perform the cloning
   *
   * @param ws ::  MDEventWorkspace to clone
   */
  template<typename MDE, size_t nd>
  void CloneMDEventWorkspace::doClone(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    std::string outWSName = getPropertyValue("OutputWorkspace");
    Progress prog(this, 0.0, 10.0, 100);
    BoxController_sptr bc = ws->getBoxController();

    if (!bc) throw std::runtime_error("Error with InputWorkspace: no BoxController!");
    if (bc->isFileBacked())
    {
      // Generate a new filename to copy to
      prog.report("Copying File");
      std::string originalFile = bc->getFilename();
      Poco::Path path = Poco::Path(originalFile).absolute();
      std::string newName = path.getBaseName() + "_clone." + path.getExtension();
      path.setFileName(newName);

      // Perform the copying
      g_log.notice() << "Cloned workspace file being copied to: " << path.toString() << std::endl;
      Poco::File(originalFile).copyTo(path.toString());
      g_log.information() << "File copied successfully." << std::endl;

      // Now load it back
      IAlgorithm_sptr alg = createSubAlgorithm("LoadMDEW", 0.5, 1.0, true);
      alg->setPropertyValue("Filename", path.toString());
      alg->setPropertyValue("FileBackEnd", "1");
      alg->setPropertyValue("Memory", "0"); //TODO: How much memory?
      alg->setPropertyValue("OutputWorkspace", outWSName);
      alg->executeAsSubAlg();

      // Set the output workspace to this
      IMDEventWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
      this->setProperty("OutputWorkspace", outWS);
    }
    else
    {
      // Perform the clone in memory.
    }
  }


  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void CloneMDEventWorkspace::exec()
  {
    IMDEventWorkspace_sptr inWS = getProperty("InputWorkspace");

    CALL_MDEVENT_FUNCTION(this->doClone, inWS);
  }



} // namespace Mantid
} // namespace MDEvents

