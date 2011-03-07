#include "MantidCrystal/LoadPeaksFile.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"

namespace Mantid
{
namespace Crystal
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(LoadPeaksFile)
  
  using namespace Mantid::Kernel;
  using namespace Mantid::API;
  using namespace Mantid::DataObjects;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LoadPeaksFile::LoadPeaksFile()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LoadPeaksFile::~LoadPeaksFile()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void LoadPeaksFile::initDocs()
  {
    this->setWikiSummary("Load an ISAW-style .peaks file into a [[PeaksWorkspace]].");
    this->setOptionalMessage("Load an ISAW-style .peaks file into a PeaksWorkspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void LoadPeaksFile::init()
  {
    std::vector<std::string> exts;
    exts.push_back(".peaks");
    // exts.push_back(".integrate");

    declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "Path to an ISAW-style .peaks filename.");
    declareProperty(new WorkspaceProperty<PeaksWorkspace>("OutputWorkspace","",Direction::Output), "Name of the output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void LoadPeaksFile::exec()
  {
    // Create the workspace
    PeaksWorkspace_sptr ws(new PeaksWorkspace());
    ws->setName(getPropertyValue("OutputWorkspace"));

    // This loads (appends) the peaks
    ws->append( getPropertyValue("Filename"));

    // Save it in the output
    setProperty("OutputWorkspace", boost::dynamic_pointer_cast<PeaksWorkspace>(ws));
  }



} // namespace Mantid
} // namespace Crystal

