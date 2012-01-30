/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/LoadAlgorithmFactory.h"
#include "MantidDataHandling/LoadPreNexus.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using std::string;

namespace Mantid
{
namespace DataHandling
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(LoadPreNexus)
  DECLARE_LOADALGORITHM(LoadPreNexus)

  static const string RUNINFO_PARAM("Filename");
  static const string MAP_PARAM("MappingFilename");


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LoadPreNexus::LoadPreNexus()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LoadPreNexus::~LoadPreNexus()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string LoadPreNexus::name() const { return "LoadPreNexus";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int LoadPreNexus::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string LoadPreNexus::category() const { return "DataHandling\\PreNexus;Workflow\\DataHandling";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void LoadPreNexus::initDocs()
  {
    this->setWikiSummary("Load a PreNexus file.");
    this->setOptionalMessage("Load a PreNexus file.");
  }

  //----------------------------------------------------------------------------------------------
  /// Returns the name of the property to be considered as the Filename for Load
  const char * LoadPreNexus::filePropertyName() const
  {
    return RUNINFO_PARAM.c_str();
  }

  /// do a quick check that this file can be loaded
  bool LoadPreNexus::quickFileCheck(const std::string& filePath,size_t nread,const file_header& header)
  {
    UNUSED_ARG(nread);
    UNUSED_ARG(header);

    std::string ext = extension(filePath);
    return (ext.rfind("_runinfo.xml") != std::string::npos);
  }

  /// check the structure of the file and  return a value between 0 and 100 of how much this file can be loaded
  int LoadPreNexus::fileCheck(const std::string& filePath)
  {
    return 0; // TODO
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void LoadPreNexus::init()
  {
    declareProperty(new FileProperty(RUNINFO_PARAM, "", FileProperty::Load, "_runinfo.xml"),
        "The name of the runinfo file to read, including its full or relative path.");
    declareProperty(new FileProperty(MAP_PARAM, "", FileProperty::OptionalLoad, ".dat"),
        "File containing the pixel mapping (DAS pixels to pixel IDs) file (typically INSTRUMENT_TS_YYYY_MM_DD.dat). The filename will be found automatically if not specified.");

    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void LoadPreNexus::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace Mantid
} // namespace DataHandling
