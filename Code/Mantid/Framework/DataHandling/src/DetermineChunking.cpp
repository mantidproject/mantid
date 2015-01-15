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
#include <set>
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

namespace Mantid {
namespace DataHandling {
const int NUM_EXT_PRENEXUS(1); ///< Number of prenexus extensions
/// Valid extensions for prenexus files
const std::string PRENEXUS_EXT[NUM_EXT_PRENEXUS] = {"_runinfo.xml"};
const int NUM_EXT_EVENT_NEXUS(3); ///< Number of event nexus extensions
/// Valid extensions for event nexus files
const std::string EVENT_NEXUS_EXT[NUM_EXT_EVENT_NEXUS] = {"_event.nxs", ".nxs",
                                                          ".nxs.h5"};
const int NUM_EXT_HISTO_NEXUS(1); ///< Number of histogram nexus extensions
/// Valid extensions for histogram nexus files
const std::string HISTO_NEXUS_EXT[NUM_EXT_HISTO_NEXUS] = {"_histo.nxs"};
const int NUM_EXT_RAW(1); ///< Number of raw file extensions
/// Valid extensions for ISIS raw files
const std::string RAW_EXT[NUM_EXT_RAW] = {".raw"};

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(DetermineChunking)

//----------------------------------------------------------------------------------------------
/// Constructor
DetermineChunking::DetermineChunking() {}

//----------------------------------------------------------------------------------------------
/// Destructor
DetermineChunking::~DetermineChunking() {}

//----------------------------------------------------------------------------------------------
/// @copydoc Mantid::API::IAlgorithm::name()
const std::string DetermineChunking::name() const {
  return "DetermineChunking";
}

/// @copydoc Mantid::API::IAlgorithm::version()
int DetermineChunking::version() const { return 1; }

/// @copydoc Mantid::API::IAlgorithm::category()
const std::string DetermineChunking::category() const {
  return "DataHandling\\PreNexus;Workflow\\DataHandling";
}

//----------------------------------------------------------------------------------------------
/// @copydoc Mantid::API::Algorithm::init()
void DetermineChunking::init() {
  // runfile to read in
  std::set<std::string> exts_set;
  exts_set.insert(PRENEXUS_EXT, PRENEXUS_EXT + NUM_EXT_PRENEXUS);
  exts_set.insert(EVENT_NEXUS_EXT, EVENT_NEXUS_EXT + NUM_EXT_EVENT_NEXUS);
  exts_set.insert(HISTO_NEXUS_EXT, HISTO_NEXUS_EXT + NUM_EXT_HISTO_NEXUS);
  exts_set.insert(RAW_EXT, RAW_EXT + NUM_EXT_RAW);
  std::vector<std::string> exts(exts_set.begin(), exts_set.end());
  this->declareProperty(
      new FileProperty("Filename", "", FileProperty::Load, exts),
      "The name of the event nexus, runinfo.xml, raw, or histo nexus file to "
      "read, including its full or relative path. The Event NeXus file name is "
      "typically of the form INST_####_event.nxs (N.B. case sensitive if "
      "running on Linux).");

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("MaxChunkSize", EMPTY_DBL(), mustBePositive,
                  "Get chunking strategy for chunks with this number of "
                  "Gbytes. File will not be loaded if this option is set.");

  declareProperty(new WorkspaceProperty<API::ITableWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/// @copydoc Mantid::API::Algorithm::exec()
void DetermineChunking::exec() {
  // get the chunking parameter and fix it up
  double maxChunk = this->getProperty("MaxChunkSize");
  if (maxChunk == 0) {
    g_log.debug() << "Converting maxChunk=0 to maxChunk=EMPTY_DBL\n";
    maxChunk = EMPTY_DBL();
  }

  // get the filename and determine the file type
  double filesize = 0;
  int m_numberOfSpectra = 0;
  string filename = this->getPropertyValue("Filename");
  FileType fileType = getFileType(filename);

  // setup the chunking table with the correct column headings
  Mantid::API::ITableWorkspace_sptr strategy =
      Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  if (fileType == PRENEXUS_FILE || fileType == EVENT_NEXUS_FILE) {
    strategy->addColumn("int", "ChunkNumber");
    strategy->addColumn("int", "TotalChunks");
  } else if (fileType == RAW_FILE || fileType == HISTO_NEXUS_FILE) {
    strategy->addColumn("int", "SpectrumMin");
    strategy->addColumn("int", "SpectrumMax");
  }
  this->setProperty("OutputWorkspace", strategy);

#ifndef MPI_BUILD
  // mpi needs work for every core, so don't do this
  if (maxChunk == 0 || isEmpty(maxChunk)) {
    return;
  }
#endif

  // --------------------- DETERMINE NUMBER OF CHUNKS
  // PreNexus
  if (fileType == PRENEXUS_FILE) {
    vector<string> eventFilenames;
    string dataDir;
    LoadPreNexus lp;
    lp.parseRuninfo(filename, dataDir, eventFilenames);
    for (size_t i = 0; i < eventFilenames.size(); i++) {
      BinaryFile<DasEvent> eventfile(dataDir + eventFilenames[i]);
      // Factor of 2 for compression
      filesize += static_cast<double>(eventfile.getNumElements()) * 48.0 /
                  (1024.0 * 1024.0 * 1024.0);
    }
  }
  // Event Nexus
  else if (fileType == EVENT_NEXUS_FILE) {

    // top level file information
    ::NeXus::File file(filename);
    std::string m_top_entry_name = setTopEntryName(filename);

    // Start with the base entry
    file.openGroup(m_top_entry_name, "NXentry");

    // Now we want to go through all the bankN_event entries
    map<string, string> entries = file.getEntries();
    map<string, string>::const_iterator it = entries.begin();
    std::string classType = "NXevent_data";
    size_t total_events = 0;
    for (; it != entries.end(); ++it) {
      std::string entry_name(it->first);
      std::string entry_class(it->second);
      if (entry_class == classType) {
        if (!isEmpty(maxChunk)) {
          try {
            // Get total number of events for each bank
            file.openGroup(entry_name, entry_class);
            file.openData("total_counts");
            if (file.getInfo().type == NX_UINT64) {
              std::vector<uint64_t> bank_events;
              file.getData(bank_events);
              total_events += bank_events[0];
            } else {
              std::vector<int> bank_events;
              file.getDataCoerce(bank_events);
              total_events += bank_events[0];
            }
            file.closeData();
            file.closeGroup();
          } catch (::NeXus::Exception &) {
            g_log.error()
                << "Unable to find total counts to determine chunking strategy."
                << std::endl;
          }
        }
      }
    }

    // Close up the file
    file.closeGroup();
    file.close();
    // Factor of 2 for compression
    filesize =
        static_cast<double>(total_events) * 48.0 / (1024.0 * 1024.0 * 1024.0);
  } else if (fileType == RAW_FILE) {
    // Check the size of the file loaded
    Poco::File info(filename);
    filesize = double(info.getSize()) * 24.0 / (1024.0 * 1024.0 * 1024.0);
    g_log.notice() << "Wksp size is " << filesize << " GB" << std::endl;

    LoadRawHelper helper;
    FILE *file = helper.openRawFile(filename);
    ISISRAW iraw;
    iraw.ioRAW(file, true);

    // Read in the number of spectra in the RAW file
    m_numberOfSpectra = iraw.t_nsp1;
    g_log.notice() << "Spectra size is " << m_numberOfSpectra << " spectra"
                   << std::endl;
    fclose(file);
  }
  // Histo Nexus
  else if (fileType == HISTO_NEXUS_FILE) {
    // Check the size of the file loaded
    Poco::File info(filename);
    filesize = double(info.getSize()) * 144.0 / (1024.0 * 1024.0 * 1024.0);
    g_log.notice() << "Wksp size is " << filesize << " GB" << std::endl;
    LoadTOFRawNexus lp;
    lp.signalNo = 1;
    // Find the entry name we want.
    std::string entry_name = LoadTOFRawNexus::getEntryName(filename);
    std::vector<std::string> bankNames;
    lp.countPixels(filename, entry_name, bankNames);
    m_numberOfSpectra = static_cast<int>(lp.numPixels);
    g_log.notice() << "Spectra size is " << m_numberOfSpectra << " spectra"
                   << std::endl;
  } else {
    throw(std::invalid_argument("unsupported file type"));
  }

  int numChunks = 0;
  if (maxChunk) // protect from divide by zero
  {
    numChunks = static_cast<int>(filesize / maxChunk);
  }

  numChunks++; // So maxChunkSize is not exceeded
  if (numChunks <= 1 || isEmpty(maxChunk)) {
#ifdef MPI_BUILD
    numChunks = 1;
#else
    g_log.information()
        << "Everything can be done in a single chunk returning empty table\n";
    return;
#endif
  }

// --------------------- FILL IN THE CHUNKING TABLE
#ifdef MPI_BUILD
  // use all cores so number of chunks should be a multiple of cores
  if (mpi::communicator().size() > 1) {
    int imult = numChunks / mpi::communicator().size() + 1;
    numChunks = imult * mpi::communicator().size();
  }
#endif

  for (int i = 1; i <= numChunks; i++) {
#ifdef MPI_BUILD
    if (mpi::communicator().size() > 1) {
      // chunk 1 should go to rank=0, chunk 2 to rank=1, etc.
      if ((i - 1) % mpi::communicator().size() != mpi::communicator().rank())
        continue;
    }
#endif
    Mantid::API::TableRow row = strategy->appendRow();
    if (fileType == PRENEXUS_FILE || fileType == EVENT_NEXUS_FILE) {
      row << i << numChunks;
    } else if (fileType == RAW_FILE || fileType == HISTO_NEXUS_FILE) {
      int spectraPerChunk = m_numberOfSpectra / numChunks;
      int first = (i - 1) * spectraPerChunk + 1;
      int last = first + spectraPerChunk - 1;
      if (i == numChunks)
        last = m_numberOfSpectra;
      row << first << last;
    }
  }
}

/// set the name of the top level NXentry m_top_entry_name
std::string DetermineChunking::setTopEntryName(std::string filename) {
  std::string top_entry_name;
  typedef std::map<std::string, std::string> string_map_t;
  try {
    string_map_t::const_iterator it;
    ::NeXus::File file = ::NeXus::File(filename);
    string_map_t entries = file.getEntries();

    // Choose the first entry as the default
    top_entry_name = entries.begin()->first;

    for (it = entries.begin(); it != entries.end(); ++it) {
      if (((it->first == "entry") || (it->first == "raw_data_1")) &&
          (it->second == "NXentry")) {
        top_entry_name = it->first;
        break;
      }
    }
  } catch (const std::exception &) {
    g_log.error()
        << "Unable to determine name of top level NXentry - assuming \"entry\"."
        << std::endl;
    top_entry_name = "entry";
  }
  return top_entry_name;
}

/**
 * Determine the file type using the filename. This throws an exception
 * if the type cannot be determined.
 *
 * @param filename Name of the file to determine the type of.
 * @return The file type.
 */
FileType DetermineChunking::getFileType(const string &filename) {
  // check for prenexus
  for (int i = 0; i < NUM_EXT_PRENEXUS; ++i) {
    if (filename.find(PRENEXUS_EXT[i]) != std::string::npos) {
      g_log.information() << "Determined \'" << filename
                          << "\' is a prenexus file\n";
      return PRENEXUS_FILE;
    }
  }

  // check for histogram nexus
  for (int i = 0; i < NUM_EXT_HISTO_NEXUS; ++i) {
    if (filename.find(HISTO_NEXUS_EXT[i]) != std::string::npos) {
      g_log.information() << "Determined \'" << filename
                          << "\' is a  histogram nexus file\n";
      return HISTO_NEXUS_FILE;
    }
  }

  // check for event nexus - must be last because a valid extension is ".nxs"
  for (int i = 0; i < NUM_EXT_EVENT_NEXUS; ++i) {
    if (filename.find(EVENT_NEXUS_EXT[i]) != std::string::npos) {
      g_log.information() << "Determined \'" << filename
                          << "\' is an event nexus file\n";
      return EVENT_NEXUS_FILE;
    }
  }

  // check for isis raw files
  for (int i = 0; i < NUM_EXT_RAW; ++i) {
    if (filename.find(RAW_EXT[i]) != std::string::npos) {
      g_log.information() << "Determined \'" << filename
                          << "\' is an ISIS raw file\n";
      return RAW_FILE;
    }
  }

  throw std::invalid_argument("Unsupported file type");
}
} // namespace Mantid
} // namespace DataHandling
