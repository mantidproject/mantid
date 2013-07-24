/*WIKI*
Workflow algorithm to determine chunking strategy 
for event nexus, runinfo.xml, raw, or histo nexus files
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
#include "MantidAPI/FileProperty.h"
#include "MantidDataHandling/DetermineChunking.h"
#include "MantidDataHandling/LoadPreNexus.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidDataHandling/LoadTOFRawNexus.h"
#include "LoadRaw/isisraw.h"
#include "MantidDataHandling/LoadRawHelper.h"
#include "MantidKernel/System.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/BinaryFile.h"
#include "MantidKernel/BoundedValidator.h"
#ifdef MPI_BUILD
    #include <boost/mpi.hpp>
    namespace mpi = boost::mpi;
#endif

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
	// Register the algorithm into the AlgorithmFactory
	DECLARE_ALGORITHM(DetermineChunking)


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
    this->setWikiSummary("Workflow algorithm to determine chunking strategy for event nexus, runinfo.xml, raw, or histo nexus files.");
    this->setOptionalMessage("Workflow algorithm to determine chunking strategy for event nexus, runinfo.xml, raw, or histo nexus files.");
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
    exts.push_back(".nxs.h5");
    exts.push_back(".raw");
    this->declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "The name of the event nexus, runinfo.xml, raw, or histo nexus file to read, including its full or relative path. The Event NeXus file name is typically of the form INST_####_event.nxs (N.B. case sensitive if running on Linux)." );

    auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
    mustBePositive->setLower(0.0);
    declareProperty("MaxChunkSize", EMPTY_DBL(), mustBePositive,
        "Get chunking strategy for chunks with this number of Gbytes. File will not be loaded if this option is set.");

    declareProperty(new WorkspaceProperty<API::ITableWorkspace>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /// @copydoc Mantid::API::Algorithm::exec()
  void DetermineChunking::exec()
  {
    double maxChunk = this->getProperty("MaxChunkSize");
    if (maxChunk == 0)
    {
      g_log.debug() << "Converting maxChunk=0 to maxChunk=EMPTY_DBL\n";
      maxChunk = EMPTY_DBL();
    }

    double filesize = 0;
    int m_numberOfSpectra = 0;
    string runinfo = this->getPropertyValue("Filename");
    std::string ext = extension(runinfo);
    Mantid::API::ITableWorkspace_sptr strategy = Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
    if( ext.compare("xml") == 0 || runinfo.compare(runinfo.size()-10,10,"_event.nxs") == 0 || runinfo.compare(runinfo.size()-7,7,".nxs.h5") == 0)
    {
		strategy->addColumn("int","ChunkNumber");
		strategy->addColumn("int","TotalChunks");
    }
    else if( ext.compare("raw") == 0|| runinfo.compare(runinfo.size()-10,10,"_histo.nxs") == 0)
    {
    	strategy->addColumn("int","SpectrumMin");
    	strategy->addColumn("int","SpectrumMax");
    }
    this->setProperty("OutputWorkspace", strategy);
#ifndef MPI_BUILD
    // mpi needs work for every core, so don't do this
    if (maxChunk == 0 || isEmpty(maxChunk))
    {
      return;
    }
#endif
    //PreNexus
    if( ext.compare("xml") == 0)
    {
      vector<string> eventFilenames;
      string dataDir;
      LoadPreNexus lp;
      lp.parseRuninfo(runinfo, dataDir, eventFilenames);
      for (size_t i = 0; i < eventFilenames.size(); i++) {
        Mantid::Kernel::BinaryFile<DasEvent> * eventfile = new BinaryFile<DasEvent>(dataDir + eventFilenames[i]);
        // Factor of 2 for compression
        filesize += static_cast<double>(eventfile->getNumElements()) * 48.0 / (1024.0*1024.0*1024.0);
      }
    }
    //Event Nexus
    else if( runinfo.compare(runinfo.size()-10,10,"_event.nxs") == 0 || runinfo.compare(runinfo.size()-7,7,".nxs.h5") == 0)
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
      // Factor of 2 for compression
      filesize = static_cast<double>(total_events) * 48.0 / (1024.0*1024.0*1024.0);
    }
    //Histo Nexus
    else if( ext.compare("raw") == 0)
    {
        // Check the size of the file loaded
        Poco::File info(runinfo);
        filesize = double(info.getSize()) * 24.0 / (1024.0*1024.0*1024.0);
        g_log.notice() << "Wksp size is " << filesize << " GB" << std::endl;

        LoadRawHelper *helper = new LoadRawHelper;
        FILE* file = helper->openRawFile(runinfo);
        ISISRAW iraw;
        iraw.ioRAW(file, true);

        // Read in the number of spectra in the RAW file
        m_numberOfSpectra = iraw.t_nsp1;
        g_log.notice() << "Spectra size is " << m_numberOfSpectra << " spectra" << std::endl;
    }
    else if( runinfo.compare(runinfo.size()-10,10,"_histo.nxs") == 0)
    {
        // Check the size of the file loaded
        Poco::File info(runinfo);
        filesize = double(info.getSize()) * 144.0 / (1024.0*1024.0*1024.0);
        g_log.notice() << "Wksp size is " << filesize << " GB" << std::endl;
        LoadTOFRawNexus lp;
        lp.signalNo = 1;
	    // Find the entry name we want.
	    std::string entry_name = LoadTOFRawNexus::getEntryName(runinfo);
	    std::vector<std::string> bankNames;
	    lp.countPixels(runinfo, entry_name, bankNames);
	    m_numberOfSpectra = static_cast<int>(lp.numPixels);
        g_log.notice() << "Spectra size is " << m_numberOfSpectra << " spectra" << std::endl;
    }
    else
    {
    	throw(std::invalid_argument("unsupported file type"));
    }

    int numChunks = static_cast<int>(filesize/maxChunk);
    numChunks ++; //So maxChunkSize is not exceeded 
    if (numChunks <= 1 || isEmpty(maxChunk))
    {
#ifdef MPI_BUILD
      numChunks = 1;
#else
      g_log.information() << "Everything can be done in a single chunk returning empty table\n";
      return;
#endif
    }

#ifdef MPI_BUILD
      // use all cores so number of chunks should be a multiple of cores
    if (mpi::communicator().size() > 1)
    {
      int imult = numChunks/mpi::communicator().size() + 1;
      numChunks = imult * mpi::communicator().size();
    }
#endif

    for (int i = 1; i <= numChunks; i++)
    {
#ifdef MPI_BUILD
	if (mpi::communicator().size() > 1)
	{
      // chunk 1 should go to rank=0, chunk 2 to rank=1, etc.
      if((i-1)%mpi::communicator().size() != mpi::communicator().rank()) continue;
	}
#endif
      Mantid::API::TableRow row = strategy->appendRow();
      if( ext.compare("xml") == 0 || runinfo.compare(runinfo.size()-10,10,"_event.nxs") == 0 || runinfo.compare(runinfo.size()-7,7,".nxs.h5") == 0)
      {
    	  row << i << numChunks;
      }
      else if( ext.compare("raw") == 0 || runinfo.compare(runinfo.size()-10,10,"_histo.nxs") == 0)
      {
    	  int spectraPerChunk = m_numberOfSpectra/numChunks;
    	  int first = (i-1) * spectraPerChunk + 1;
    	  int last = first + spectraPerChunk - 1;
    	  if (i == numChunks) last = m_numberOfSpectra;
    	  row << first << last;
      }
    }
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

  /** returns the extension of the given file
   *  @param fileName :: name of the file.
   */
  std::string DetermineChunking::extension(const std::string& fileName)
  {
    std::size_t pos=fileName.find_last_of(".");
    if(pos==std::string::npos)
    {
      return"" ;
    }
    std::string extn=fileName.substr(pos+1,fileName.length()-pos);
    std::transform(extn.begin(),extn.end(),extn.begin(),tolower);
    return extn;
  }


} // namespace Mantid
} // namespace DataHandling
