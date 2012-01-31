/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include <fstream>
#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/SAX/InputSource.h>
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/LoadAlgorithmFactory.h"
#include "MantidDataHandling/LoadPreNexus.h"
#include "MantidKernel/System.h"
#include "MantidKernel/VisibleWhenProperty.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using std::string;
using std::vector;

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
  const std::string LoadPreNexus::name() const
  {
    return "LoadPreNexus";
  }
  
  /// Algorithm's version for identification. @see Algorithm::version
  int LoadPreNexus::version() const
  {
    return 1;
  }
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string LoadPreNexus::category() const
  {
    return "DataHandling\\PreNexus;Workflow\\DataHandling";
  }

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
    // runfile to read in
    declareProperty(new FileProperty(RUNINFO_PARAM, "", FileProperty::Load, "_runinfo.xml"),
        "The name of the runinfo file to read, including its full or relative path.");

    // copied (by hand) from LoadEventPreNexus2
    declareProperty(new FileProperty(MAP_PARAM, "", FileProperty::OptionalLoad, ".dat"),
        "File containing the pixel mapping (DAS pixels to pixel IDs) file (typically INSTRUMENT_TS_YYYY_MM_DD.dat). The filename will be found automatically if not specified.");
    BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
    mustBePositive->setLower(1);
    declareProperty("ChunkNumber", EMPTY_INT(), mustBePositive,
        "If loading the file by sections ('chunks'), this is the section number of this execution of the algorithm.");
    declareProperty("TotalChunks", EMPTY_INT(), mustBePositive->clone(),
        "If loading the file by sections ('chunks'), this is the total number of sections.");
    // TotalChunks is only meaningful if ChunkNumber is set
    // Would be nice to be able to restrict ChunkNumber to be <= TotalChunks at validation
    setPropertySettings("TotalChunks", new VisibleWhenProperty(this, "ChunkNumber", IS_NOT_DEFAULT));
    std::vector<std::string> propOptions;
    propOptions.push_back("Auto");
    propOptions.push_back("Serial");
    propOptions.push_back("Parallel");
    declareProperty("UseParallelProcessing", "Auto",new ListValidator(propOptions),
        "Use multiple cores for loading the data?\n"
        "  Auto: Use serial loading for small data sets, parallel for large data sets.\n"
        "  Serial: Use a single core.\n"
        "  Parallel: Use all available cores.");

    declareProperty(new PropertyWithValue<bool>("LoadMonitors", true, Direction::Input),
                    "Load the monitors from the file.");


    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void LoadPreNexus::exec()
  {
    string runinfo = this->getPropertyValue(RUNINFO_PARAM);
    string mapfile = this->getPropertyValue(MAP_PARAM);
    int chunkNumber = this->getProperty("ChunkNumber");
    int chunkTotal = this->getProperty("TotalChunks");
    string useParallel = this->getProperty("UseParallelProcessing");
    string wsname = this->getProperty("OutputWorkspace");
    bool loadmonitors = this->getProperty("LoadMonitors");

    // determine the event file names
    vector<string> eventFilenames;
    string dataDir;
    this->parseRuninfo(runinfo, dataDir, eventFilenames);

    // load event files
    IEventWorkspace_sptr outws;
    string temp_wsname;

    for (size_t i = 0; i < eventFilenames.size(); i++) {
      if (i == 0)
        temp_wsname = wsname;
      else
        temp_wsname = "__" + wsname + "_temp__";

      IAlgorithm_sptr alg = this->createSubAlgorithm("LoadEventPreNexus");
      alg->setProperty("EventFilename", dataDir + eventFilenames[i]);
      alg->setProperty("MappingFilename", mapfile);
      alg->setProperty("ChunkNumber", chunkNumber);
      alg->setProperty("TotalChunks", chunkTotal);
      alg->setProperty("UseParallelProcessing", useParallel);
      alg->setPropertyValue("OutputWorkspace", temp_wsname);
      alg->executeAsSubAlg();

      if (i == 0)
      {
        outws = alg->getProperty("OutputWorkspace");
      }
      else
      {
        IEventWorkspace_sptr tempws = alg->getProperty("OutputWorkspace");
        // clean up properties before adding data
        Run & run = tempws->mutableRun();
        if (run.hasProperty("gd_prtn_chrg"))
          run.removeProperty("gd_prtn_chrg");
        if (run.hasProperty("proton_charge"))
          run.removeProperty("proton_charge");

        outws += tempws;
      }
    }


    // load the logs
    this->runLoadNexusLogs(runinfo, dataDir, outws);

    // publish output workspace
    this->setProperty("OutputWorkspace", outws);

    // load the monitor
    //TODO update progress
    if (loadmonitors)
    {
      this->runLoadMonitors();
    }
  }

  void LoadPreNexus::parseRuninfo(const string &runinfo, string &dataDir, vector<string> &eventFilenames)
  {
    eventFilenames.clear();

    // Create a Poco Path object for runinfo filename
    Poco::Path runinfoPath(runinfo, Poco::Path::PATH_GUESS);
    // Now lets get the directory
    Poco::Path dirPath(runinfoPath.parent());
    dataDir = dirPath.absolute().toString();
    g_log.debug() << "Data directory \"" << dataDir << "\"\n";

    std::ifstream in(runinfo.c_str());
    Poco::XML::InputSource src(in);

    Poco::XML::DOMParser parser;
    Poco::AutoPtr<Poco::XML::Document> pDoc = parser.parse(&src);

    Poco::XML::NodeIterator it(pDoc, Poco::XML::NodeFilter::SHOW_ELEMENT);
    Poco::XML::Node* pNode = it.nextNode(); // root node
    while (pNode)
    {
      if (pNode->nodeName() == "RunInfo") // standard name for this type
      {
        pNode = pNode->firstChild();
        while (pNode)
        {
          if (pNode->nodeName() == "FileList")
          {
            pNode = pNode->firstChild();
            while (pNode)
            {
              if (pNode->nodeName() == "DataList")
              {
                pNode = pNode->firstChild();
                while (pNode)
                {
                  if (pNode->nodeName() == "scattering")
                  {
                    Poco::XML::Element* element = static_cast<Poco::XML::Element*> (pNode);
                    eventFilenames.push_back(element->getAttribute("name"));
                  }
                  pNode = pNode->nextSibling();
                }
              }
              else // search for DataList
                pNode = pNode->nextSibling();
            }
          }
          else // search for FileList
            pNode = pNode->nextSibling();
        }
      }
      else // search for RunInfo
        pNode = pNode->nextSibling();
    }

    // report the results to the log
    if (eventFilenames.size() == 1)
    {
      g_log.debug() << "Found 1 event file: \"" << eventFilenames[0] << "\"\n";
    }
    else
    {
      g_log.debug() << "Found " << eventFilenames.size() << " event files:";
      for (size_t i = 0; i < eventFilenames.size(); i++)
      {
        g_log.debug() << "\"" << eventFilenames[i] << "\" ";
      }
      g_log.debug() << "\n";
    }
  }

  void LoadPreNexus::runLoadNexusLogs(const string &runinfo, const string &dataDir, IEventWorkspace_sptr wksp)
  {
    // determine the name of the file "inst_run"
    string shortName = runinfo.substr(dataDir.size());
    shortName = shortName.substr(0, shortName.find("_runinfo.xml"));
    g_log.debug() << "SHORTNAME = \"" << shortName << "\"\n";

    // put together a list of possible locations
    vector<string> possibilities;
    possibilities.push_back(dataDir + shortName + "_event.nxs"); // next to runinfo
    possibilities.push_back(dataDir + shortName + "_histo.nxs");
    possibilities.push_back(dataDir + shortName + ".nxs");
    possibilities.push_back(dataDir + "../NeXus/" + shortName + "_event.nxs"); // in NeXus directory
    possibilities.push_back(dataDir + "../NeXus/" + shortName + "_histo.nxs");
    possibilities.push_back(dataDir + "../NeXus/" + shortName + ".nxs");

    // run the algorithm
    for (size_t i = 0; i < possibilities.size(); i++)
    {
      if (Poco::File(possibilities[i]).exists())
      {
        g_log.information() << "Loading logs from \"" << possibilities[i] << "\"\n";
        IAlgorithm_sptr alg = this->createSubAlgorithm("LoadNexusLogs");
        alg->setProperty("Workspace", wksp);
        alg->setProperty("Filename", possibilities[i]);
        alg->setProperty("OverwriteLogs", true); // TODO should be false
        alg->executeAsSubAlg();
      }
    }

  }

  void LoadPreNexus::runLoadMonitors()
  {
    std::string mon_wsname = this->getProperty("OutputWorkspace");
    mon_wsname.append("_monitors");

    IAlgorithm_sptr alg = this->createSubAlgorithm("LoadPreNexusMonitors");
    alg->setPropertyValue("RunInfoFilename", this->getProperty(RUNINFO_PARAM));
    alg->setPropertyValue("OutputWorkspace", mon_wsname);
    alg->executeAsSubAlg();
    MatrixWorkspace_sptr mons = alg->getProperty("OutputWorkspace");
    this->declareProperty(new WorkspaceProperty<>("MonitorWorkspace",
        mon_wsname, Direction::Output), "Monitors from the Event NeXus file");
    this->setProperty("MonitorWorkspace", mons);
  }

} // namespace Mantid
} // namespace DataHandling
