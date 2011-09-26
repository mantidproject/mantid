#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/CloneMDWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
#include <Poco/File.h>
#include <Poco/Path.h>
#include "MantidAPI/FileProperty.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(CloneMDWorkspace)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  CloneMDWorkspace::CloneMDWorkspace()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  CloneMDWorkspace::~CloneMDWorkspace()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void CloneMDWorkspace::initDocs()
  {
    this->setWikiSummary("Clones (copies) an existing MDEventWorkspace into a new one.");
    this->setOptionalMessage("Clones (copies) an existing [[MDEventWorkspace]] into a new one.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void CloneMDWorkspace::init()
  {
    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace","",Direction::Input),
        "An input MDEventWorkspace.");
    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("OutputWorkspace","",Direction::Output),
        "Name of the output MDEventWorkspace.");

    std::vector<std::string> exts(1, ".nxs");
    declareProperty(new FileProperty("Filename", "", FileProperty::OptionalSave, exts),
        "If the input workspace is file-backed, specify a file to which to save the cloned workspace.\n"
        "If the workspace is file-backed but this parameter is NOT specified, then a new filename with '_clone' appended is created next to the original file.\n"
        "No effect if the input workspace is NOT file-backed.\n"
        "");

  }

  //----------------------------------------------------------------------------------------------
  /** Perform the cloning
   *
   * @param ws ::  MDEventWorkspace to clone
   */
  template<typename MDE, size_t nd>
  void CloneMDWorkspace::doClone(const typename MDEventWorkspace<MDE, nd>::sptr ws)
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
      std::string outFilename = getPropertyValue("Filename");
      if (outFilename.empty())
      {
        // Auto-generated name
        Poco::Path path = Poco::Path(originalFile).absolute();
        std::string newName = path.getBaseName() + "_clone." + path.getExtension();
        path.setFileName(newName);
        outFilename = path.toString();
      }

      // Perform the copying
      g_log.notice() << "Cloned workspace file being copied to: " << outFilename << std::endl;
      Poco::File(originalFile).copyTo(outFilename);
      g_log.information() << "File copied successfully." << std::endl;

      // Now load it back
      IAlgorithm_sptr alg = createSubAlgorithm("LoadMD", 0.5, 1.0, false);
      alg->setPropertyValue("Filename", outFilename);
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
      boost::shared_ptr<MDEventWorkspace<MDE,nd> > outWS(new MDEventWorkspace<MDE,nd>(*ws));
      this->setProperty("OutputWorkspace", boost::dynamic_pointer_cast<IMDEventWorkspace>(outWS) );
    }
  }


  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void CloneMDWorkspace::exec()
  {
    IMDEventWorkspace_sptr inWS = getProperty("InputWorkspace");

    CALL_MDEVENT_FUNCTION(this->doClone, inWS);
  }



} // namespace Mantid
} // namespace MDEvents

