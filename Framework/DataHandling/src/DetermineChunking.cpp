// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/DetermineChunking.h"
#include "LoadRaw/isisraw.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadPreNexus.h"
#include "MantidDataHandling/LoadRawHelper.h"
#include "MantidDataHandling/LoadTOFRawNexus.h"
#include "MantidKernel/BinaryFile.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidNexus/NeXusException.hpp"
#include "MantidNexus/NeXusFile.hpp"

#include <Poco/File.h>
#include <exception>
#include <set>
#include <vector>

using namespace ::NeXus;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using std::map;
using std::size_t;
using std::string;
using std::vector;

namespace Mantid::DataHandling {
/// Valid extensions for prenexus files
const std::vector<std::string> PRENEXUS_EXT = {"_runinfo.xml"};
/// Valid extensions for event nexus files
const std::vector<std::string> EVENT_NEXUS_EXT = {"_event.nxs", ".nxs", ".nxs.h5"};
/// Valid extensions for histogram nexus files
const std::vector<std::string> HISTO_NEXUS_EXT = {"_histo.nxs"};
/// Valid extensions for ISIS raw files
const std::vector<std::string> RAW_EXT = {".raw"};

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(DetermineChunking)

namespace {
constexpr double BYTES_TO_GiB = 1. / 1024. / 1024. / 1024.;
}

//----------------------------------------------------------------------------------------------
/// @copydoc Mantid::API::IAlgorithm::name()
const std::string DetermineChunking::name() const { return "DetermineChunking"; }

/// @copydoc Mantid::API::IAlgorithm::version()
int DetermineChunking::version() const { return 1; }

/// @copydoc Mantid::API::IAlgorithm::category()
const std::string DetermineChunking::category() const { return "DataHandling\\PreNexus;Workflow\\DataHandling"; }

//----------------------------------------------------------------------------------------------
/// @copydoc Mantid::API::Algorithm::init()
void DetermineChunking::init() {
  // runfile to read in
  std::set<std::string> exts_set;
  exts_set.insert(PRENEXUS_EXT.cbegin(), PRENEXUS_EXT.cend());
  exts_set.insert(EVENT_NEXUS_EXT.cbegin(), EVENT_NEXUS_EXT.cend());
  exts_set.insert(HISTO_NEXUS_EXT.cbegin(), HISTO_NEXUS_EXT.cend());
  exts_set.insert(RAW_EXT.cbegin(), RAW_EXT.end());
  std::vector<std::string> exts(exts_set.begin(), exts_set.end());
  this->declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, exts),
                        "The name of the event nexus, runinfo.xml, raw, or histo nexus file to "
                        "read, including its full or relative path. The Event NeXus file name is "
                        "typically of the form INST_####_event.nxs (N.B. case sensitive if "
                        "running on Linux).");

  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("MaxChunkSize", EMPTY_DBL(), mustBePositive,
                  "Get chunking strategy for chunks with this number of "
                  "Gbytes. File will not be loaded if this option is set.");

  declareProperty(std::make_unique<WorkspaceProperty<API::ITableWorkspace>>("OutputWorkspace", "", Direction::Output),
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
  int m_numberOfSpectra = 0;
  string filename = this->getPropertyValue("Filename");
  FileType fileType = getFileType(filename);

  // setup the chunking table with the correct column headings
  Mantid::API::ITableWorkspace_sptr strategy = Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  if (fileType == PRENEXUS_FILE || fileType == EVENT_NEXUS_FILE) {
    strategy->addColumn("int", "ChunkNumber");
    strategy->addColumn("int", "TotalChunks");
  } else if (fileType == RAW_FILE || fileType == HISTO_NEXUS_FILE) {
    strategy->addColumn("int", "SpectrumMin");
    strategy->addColumn("int", "SpectrumMax");
  }
  this->setProperty("OutputWorkspace", strategy);

  if (maxChunk == 0 || isEmpty(maxChunk)) {
    return;
  }

  Poco::File fileinfo(filename);
  const double fileSizeGiB = static_cast<double>(fileinfo.getSize()) * BYTES_TO_GiB;

  // don't bother opening the file if its size is "small"
  // note that prenexus "_runinfo.xml" files don't represent what
  // is actually loaded
  if (fileType != PRENEXUS_FILE && 6. * fileSizeGiB < maxChunk)
    return;

  // --------------------- DETERMINE NUMBER OF CHUNKS
  double wkspSizeGiB = 0;
  // PreNexus
  if (fileType == PRENEXUS_FILE) {
    vector<string> eventFilenames;
    string dataDir;
    LoadPreNexus lp;
    lp.parseRuninfo(filename, dataDir, eventFilenames);
    for (const auto &eventFilename : eventFilenames) {
      BinaryFile<DasEvent> eventfile(dataDir + eventFilename);
      // Factor of 2 for compression
      wkspSizeGiB += static_cast<double>(eventfile.getNumElements()) * 48.0 * BYTES_TO_GiB;
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
      const std::string entry_class(it->second);
      if (entry_class == classType) {
        if (!isEmpty(maxChunk)) {
          try {
            const std::string entry_name(it->first);
            // Get total number of events for each bank
            file.openGroup(entry_name, entry_class);
            file.openData("total_counts");
            if (file.getInfo().type == NXnumtype::UINT64) {
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
            g_log.error() << "Unable to find total counts to determine "
                             "chunking strategy.\n";
          }
        }
      }
    }

    // Close up the file
    file.closeGroup();
    file.close();
    // Factor of 2 for compression
    wkspSizeGiB = static_cast<double>(total_events) * 48.0 * BYTES_TO_GiB;
  } else if (fileType == RAW_FILE) {
    // Check the size of the file loaded
    wkspSizeGiB = fileSizeGiB * 24.0;
    g_log.notice() << "Wksp size is " << wkspSizeGiB << " GB\n";

    LoadRawHelper helper;
    FILE *file = helper.openRawFile(filename);
    ISISRAW iraw;
    iraw.ioRAW(file, true);

    // Read in the number of spectra in the RAW file
    m_numberOfSpectra = iraw.t_nsp1;
    g_log.notice() << "Spectra size is " << m_numberOfSpectra << " spectra\n";
    fclose(file);
  }
  // Histo Nexus
  else if (fileType == HISTO_NEXUS_FILE) {
    // Check the size of the file loaded
    wkspSizeGiB = fileSizeGiB * 144.0;
    g_log.notice() << "Wksp size is " << wkspSizeGiB << " GB\n";
    LoadTOFRawNexus lp;
    lp.m_signalNo = 1;
    // Find the entry name we want.
    std::string entry_name = LoadTOFRawNexus::getEntryName(filename);
    std::vector<std::string> bankNames;
    lp.countPixels(filename, entry_name, bankNames);
    m_numberOfSpectra = static_cast<int>(lp.m_numPixels);
    g_log.notice() << "Spectra size is " << m_numberOfSpectra << " spectra\n";
  } else {
    throw(std::invalid_argument("unsupported file type"));
  }

  int numChunks = 0;
  if (maxChunk != 0.0) // protect from divide by zero
  {
    numChunks = static_cast<int>(wkspSizeGiB / maxChunk);
  }

  numChunks++; // So maxChunkSize is not exceeded
  if (numChunks <= 1 || isEmpty(maxChunk)) {
    g_log.information() << "Everything can be done in a single chunk returning empty table\n";
    return;
  }

  // --------------------- FILL IN THE CHUNKING TABLE
  for (int i = 1; i <= numChunks; i++) {
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
std::string DetermineChunking::setTopEntryName(const std::string &filename) {
  std::string top_entry_name;
  using string_map_t = std::map<std::string, std::string>;
  try {
    string_map_t::const_iterator it;
    ::NeXus::File file = ::NeXus::File(filename);
    string_map_t entries = file.getEntries();

    // Choose the first entry as the default
    top_entry_name = entries.begin()->first;

    for (it = entries.begin(); it != entries.end(); ++it) {
      if (((it->first == "entry") || (it->first == "raw_data_1")) && (it->second == "NXentry")) {
        top_entry_name = it->first;
        break;
      }
    }
  } catch (const std::exception &) {
    g_log.error() << "Unable to determine name of top level NXentry - assuming "
                     "\"entry\".\n";
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
  if (filenameHasExtension(filename, PRENEXUS_EXT)) {
    g_log.information() << "Determined \'" << filename << "\' is a prenexus file\n";
    return PRENEXUS_FILE;
  }

  // check for histogram nexus
  if (filenameHasExtension(filename, HISTO_NEXUS_EXT)) {
    g_log.information() << "Determined \'" << filename << "\' is a  histogram nexus file\n";
    return HISTO_NEXUS_FILE;
  }

  // check for event nexus - must be last because a valid extension is ".nxs"
  if (filenameHasExtension(filename, EVENT_NEXUS_EXT)) {
    g_log.information() << "Determined \'" << filename << "\' is an event nexus file\n";
    return EVENT_NEXUS_FILE;
  }

  // check for isis raw files
  if (filenameHasExtension(filename, RAW_EXT)) {
    g_log.information() << "Determined \'" << filename << "\' is an ISIS raw file\n";
    return RAW_FILE;
  }

  throw std::invalid_argument("Unsupported file type");
}

bool DetermineChunking::filenameHasExtension(const std::string &filename,
                                             const std::vector<std::string> &fileExtensions) {
  return std::any_of(fileExtensions.cbegin(), fileExtensions.cend(),
                     [&filename](const auto &extension) { return filename.find(extension) != std::string::npos; });
}

} // namespace Mantid::DataHandling
