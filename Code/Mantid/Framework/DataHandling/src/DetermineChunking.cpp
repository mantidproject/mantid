/*WIKI*
Workflow algorithm to determine chunking strategy.
*WIKI*/

#include <exception>
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
#include "MantidDataHandling/DetermineChunking.h"
#include "MantidDataHandling/LoadPreNexus.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidKernel/System.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/BinaryFile.h"

using namespace ::NeXus;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using std::size_t;
using std::map;
using std::string;
using std::vector;

namespace Mantid
{
namespace DataHandling
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(DetermineChunking)
  DECLARE_LOADALGORITHM(DetermineChunking)

  static const string RUNINFO_PARAM("Filename");


  //----------------------------------------------------------------------------------------------
  /// Constructor
  DetermineChunking::DetermineChunking()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /// Destructor
  DetermineChunking::~DetermineChunking()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// @copydoc Mantid::API::IAlgorithm::name()
  const std::string DetermineChunking::name() const
  {
    return "DetermineChunking";
  }
  
  /// @copydoc Mantid::API::IAlgorithm::version()
  int DetermineChunking::version() const
  {
    return 1;
  }
  
  /// @copydoc Mantid::API::IAlgorithm::category()
  const std::string DetermineChunking::category() const
  {
    return "DataHandling\\PreNexus;Workflow\\DataHandling";
  }

  //----------------------------------------------------------------------------------------------
  /// @copydoc Mantid::API::Algorithm::initDocs()
  void DetermineChunking::initDocs()
  {
    this->setWikiSummary("Determine chunking strategy for event nexus or runinfo.xml files.");
    this->setOptionalMessage("Determine chunking strategy for event nexus or runinfo.xml files.");
  }

  //----------------------------------------------------------------------------------------------
  /// @copydoc Mantid::API::IDataFileChecker::filePropertyName()
  const char * DetermineChunking::filePropertyName() const
  {
    return RUNINFO_PARAM.c_str();
  }

  /// @copydoc Mantid::API::IDataFileChecker::quickFileCheck
  bool DetermineChunking::quickFileCheck(const std::string& filePath,size_t nread,const file_header& header)
  {
    UNUSED_ARG(nread);
    UNUSED_ARG(header);

    std::string ext = extension(filePath);
    return (ext.rfind("_runinfo.xml") != std::string::npos);
  }

  /// @copydoc Mantid::API::IDataFileChecker::fileCheck
  int DetermineChunking::fileCheck(const std::string& filePath)
  {
    std::string ext = extension(filePath);

    if (ext.rfind("_runinfo.xml") != std::string::npos)
      return 80;
    else
      return 0;
  }

  //----------------------------------------------------------------------------------------------
  /// @copydoc Mantid::API::Algorithm::init()
  void DetermineChunking::init()
  {
    // runfile to read in
    std::vector<std::string> exts;
    exts.push_back("_runinfo.xml");
    exts.push_back("_event.nxs");
    exts.push_back(".nxs");
    this->declareProperty(new FileProperty(RUNINFO_PARAM, "", FileProperty::Load, exts),
        "The name of the runinfo file to read, including its full or relative path. \n"
        "Or the name of the Event NeXus file to read, including its full or relative path. \n"
        "The Event NeXus file name is typically of the form INST_####_event.nxs (N.B. case sensitive if running on Linux)." );

    declareProperty("MaxChunkSize", EMPTY_DBL(),
        "Get chunking strategy for chunks with this number of Gbytes. File will not be loaded if this option is set.");

    declareProperty(new WorkspaceProperty<API::ITableWorkspace>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /// @copydoc Mantid::API::Algorithm::exec()
  void DetermineChunking::exec()
  {
    double maxChunk = this->getProperty("MaxChunkSize");
    double filesize = 0;
    string runinfo = this->getPropertyValue(RUNINFO_PARAM);
    std::string ext = this->extension(runinfo);

    if( ext.compare("xml") == 0)
    {
      vector<string> eventFilenames;
      string dataDir;
      LoadPreNexus lp;
      lp.parseRuninfo(runinfo, dataDir, eventFilenames);
      for (size_t i = 0; i < eventFilenames.size(); i++) {
        Mantid::Kernel::BinaryFile<DasEvent> * eventfile = new BinaryFile<DasEvent>(dataDir + eventFilenames[i]);
        // Factor fo 2 for compression
        filesize += static_cast<double>(eventfile->getNumElements()) * 48.0 / (1024.0*1024.0*1024.0);
      }
    }
    else
    {
      // top level file information
      ::NeXus::File file(runinfo);
      std::string m_top_entry_name = setTopEntryName(runinfo);
    
      //Start with the base entry
      file.openGroup(m_top_entry_name, "NXentry");
    
      //Now we want to go through all the bankN_event entries
      typedef std::map<std::string,std::string> string_map_t; 
      map<string, string> entries = file.getEntries();
      map<string,string>::const_iterator it = entries.begin();
      std::string classType = "NXevent_data";
      size_t total_events = 0;
      double maxChunk = this->getProperty("MaxChunkSize");
      for (; it != entries.end(); ++it)
      {
        std::string entry_name(it->first);
        std::string entry_class(it->second);
        if ( entry_class == classType )
        {
          if (!isEmpty(maxChunk))
          {
            try
            {
              // Get total number of events for each bank
              file.openGroup(entry_name,entry_class);
              file.openData("total_counts");
              if ( file.getInfo().type == NX_UINT64 )
              {
                std::vector<uint64_t> bank_events;
                file.getData(bank_events);
                total_events +=bank_events[0];
              }
              else
              {
                std::vector<int> bank_events;
                file.getDataCoerce(bank_events);
                total_events +=bank_events[0];
              }
              file.closeData();
              file.closeGroup();
            }
            catch (::NeXus::Exception&)
            {
              g_log.error() << "Unable to find total counts to determine chunking strategy." << std::endl;
            }
          }
        }
      }
    
      //Close up the file
      file.closeGroup();
      file.close();
      // Factor fo 2 for compression
      filesize = static_cast<double>(total_events) * 48.0 / (1024.0*1024.0*1024.0);
    }

    int numChunks = static_cast<int>(filesize/maxChunk);
    numChunks ++; //So maxChunkSize is not exceeded 
    if (numChunks < 0) numChunks = 1;
    Mantid::API::ITableWorkspace_sptr strategy = Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
    strategy->addColumn("int","ChunkNumber");
    strategy->addColumn("int","TotalChunks");

    for (int i = 1; i <= numChunks; i++) 
    {
      Mantid::API::TableRow row = strategy->appendRow();
      row << i << numChunks;
    }
    this->setProperty("OutputWorkspace", strategy);
  }
  /// set the name of the top level NXentry m_top_entry_name
  std::string DetermineChunking::setTopEntryName(std::string m_filename)
  {
    std::string m_top_entry_name;
    typedef std::map<std::string,std::string> string_map_t; 
    try
    {
      string_map_t::const_iterator it;
      ::NeXus::File file = ::NeXus::File(m_filename);
      string_map_t entries = file.getEntries();
  
      // Choose the first entry as the default
      m_top_entry_name = entries.begin()->first;
  
      for (it = entries.begin(); it != entries.end(); ++it)
      {
        if ( ((it->first == "entry") || (it->first == "raw_data_1")) && (it->second == "NXentry") )
        {
          m_top_entry_name = it->first;
          break;
        }
      }
    }
    catch(const std::exception&)
    {
      g_log.error() << "Unable to determine name of top level NXentry - assuming \"entry\"." << std::endl;
      m_top_entry_name = "entry";
    }
    return m_top_entry_name;
  }



} // namespace Mantid
} // namespace DataHandling
