// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidLiveData/FileEventDataListener.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FileLoaderRegistry.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <algorithm>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid::LiveData {
DECLARE_LISTENER(FileEventDataListener)

namespace {
/// static logger
Kernel::Logger g_log("FileEventDataListener");
} // namespace

/// Constructor
FileEventDataListener::FileEventDataListener()
    : LiveListener(), m_filename(), m_runNumber(-1), m_tempWSname("__filelistenerchunk"), m_nextChunk(1),
      m_filePropName("Filename"), m_loaderName(""), m_canLoadMonitors(true), m_chunkload(nullptr) {
  std::string tfilename = ConfigService::Instance().getString("fileeventdatalistener.filename");
  if (tfilename.empty()) {
    g_log.error("Configuration property fileeventdatalistener.filename not "
                "found. The algorithm will fail!");
  } else {
    // If passed a filename with no path, find it. Otherwise, same file
    // will be found.
    m_filename = FileFinder::Instance().getFullPath(tfilename);
    if (m_filename.empty()) {
      g_log.error("Cannot find " + tfilename + ". The algorithm will fail.");
    } else {
      auto loader = FileLoaderRegistry::Instance().chooseLoader(m_filename);
      m_loaderName = loader->name();
      if (m_loaderName == "LoadEventPreNexus") {
        m_filePropName = "EventFilename";
        m_canLoadMonitors = false;
      } else if (m_loaderName == "LoadEventNexus") {
        m_filePropName = "Filename";
        m_canLoadMonitors = true;
      } else {
        g_log.error("No loader for " + m_filename + " that supports chunking (found loader '" + m_loaderName +
                    "'). The algorithm will fail.");
      }
    }
  }

  auto numChunks = ConfigService::Instance().getValue<int>("fileeventdatalistener.chunks");
  m_numChunks = numChunks.value_or(0);
  if (!numChunks.has_value()) {
    g_log.error("Configuration property fileeventdatalistener.chunks not "
                "found. The algorithm will fail!");
  }

  // Add an integer, incremented for each listener instance, to the temporary
  // workspace name so that multiple
  // listeners can be in existence at the same time.
  static int counter = 0;
  std::stringstream count;
  count << ++counter;
  m_tempWSname += count.str();
}

/// Destructor
FileEventDataListener::~FileEventDataListener() {
  // Don't disappear until any running job has finished or bad things happen!
  if (m_chunkload) {
    m_chunkload->wait();
  }
  // Clean up the hidden workspace if necessary
  if (AnalysisDataService::Instance().doesExist(m_tempWSname)) {
    AnalysisDataService::Instance().remove(m_tempWSname);
  }
}

bool FileEventDataListener::connect(const Poco::Net::SocketAddress & /*address*/) {
  // Do nothing for now. Later, put in stuff to help test failure modes.
  return true;
}

bool FileEventDataListener::isConnected() {
  return true; // For the time being at least
}

ILiveListener::RunStatus FileEventDataListener::runStatus() {
  // Say we're outside a run if this is called before start is
  if (m_nextChunk == 1) {
    return NoRun;
  }
  // This means the first chunk is being/has just been loaded
  else if (m_nextChunk == 2) {
    return BeginRun;
  }
  // This means we've read the whole file
  else if (m_chunkload == nullptr) {
    return EndRun;
  }
  // Otherwise we're in the run
  else
    return Running;
}

int FileEventDataListener::runNumber() const { return m_runNumber; }

void FileEventDataListener::start(Types::Core::DateAndTime /*startTime*/) // Ignore the start time
{
  // Kick off loading the first chunk (which will include loading the instrument
  // etc.)
  loadChunk();
}

std::shared_ptr<Workspace> FileEventDataListener::extractData() {
  // Once the end of the file is reached, this method throws to stop the calling
  // algorithm.
  // This is equivalent to the end of the run - which we still need to figure
  // out how to handle.
  if (m_chunkload == nullptr) {
    throw std::runtime_error("The whole file has been read!");
  }

  // If the loading of the chunk isn't finished, then we need to wait
  m_chunkload->wait();
  if (!m_chunkload->data()) {
    throw std::runtime_error(m_loaderName + " failed for some reason with file '" + m_filename + "'.");
  }
  // The loading succeeded: get the workspace from the ADS.
  MatrixWorkspace_sptr chunk = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_tempWSname);
  // Remove the workspace from the ADS now we've extracted it
  AnalysisDataService::Instance().remove(m_tempWSname);
  // Delete the ActiveResult to signify that we're done with it.
  m_chunkload = nullptr;
  // Kick off the loading of the next chunk (unless we're at the end of the
  // file)
  if (m_nextChunk <= m_numChunks) {
    loadChunk();
  } else {
    m_loader.reset(); // Clear the algorithm so that it releases its handle on
                      // the workspace
  }

  m_runNumber = chunk->getRunNumber();

  if (m_loaderName == "LoadEventNexus") {
    // Scale the proton charge by the number of chunks
    TimeSeriesProperty<double> *pcharge = chunk->mutableRun().getTimeSeriesProperty<double>("proton_charge");
    auto values = pcharge->valuesAsVector();
    std::transform(values.cbegin(), values.cend(), values.begin(),
                   [this](const auto &value) { return value / m_numChunks; });
    pcharge->replaceValues(pcharge->timesAsVector(), values);
    chunk->mutableRun().integrateProtonCharge();
  }

  return chunk;
}

/// Load the next chunk of data. Calls Algorithm::executeAsync to do it in
/// another thread.
void FileEventDataListener::loadChunk() {
  m_loader = AlgorithmManager::Instance().createUnmanaged(m_loaderName);
  m_loader->initialize();
  //    loader->setChild(true); // It can't be a child because the output needs
  //    to go in the ADS
  m_loader->setLogging(false);
  m_loader->setPropertyValue(m_filePropName, m_filename);
  m_loader->setProperty("ChunkNumber", m_nextChunk++); // post-increment
  m_loader->setProperty("TotalChunks", m_numChunks);
  if (m_canLoadMonitors) {
    m_loader->setProperty("LoadMonitors", false);
  }
  m_loader->setPropertyValue("OutputWorkspace",
                             m_tempWSname); // Goes into 'hidden' workspace
  m_chunkload = std::make_unique<Poco::ActiveResult<bool>>(m_loader->executeAsync());
}

} // namespace Mantid::LiveData
