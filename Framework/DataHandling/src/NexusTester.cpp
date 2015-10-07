#include "MantidDataHandling/NexusTester.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/ListValidator.h"

#include <Poco/File.h>
#include <Poco/Thread.h>

#include <nexus/NeXusFile.hpp>
#include <stdlib.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(NexusTester)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
NexusTester::NexusTester() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
NexusTester::~NexusTester() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string NexusTester::name() const { return "NexusTester"; }

/// Algorithm's version for identification. @see Algorithm::version
int NexusTester::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string NexusTester::category() const {
  return "Utility\\Development";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void NexusTester::init() {
  std::vector<std::string> exts;
  exts.push_back(".nxs");
  declareProperty(
      new FileProperty("SaveFilename", "", FileProperty::OptionalSave, exts),
      "The name of the Nexus file to write.");

  declareProperty(
      new FileProperty("LoadFilename", "", FileProperty::OptionalLoad, exts),
      "The name of the Nexus file to load (optional).\n"
      "Must have been written by NexusTester algorithm.");

  declareProperty("ChunkSize", 10,
                  "Chunk size for writing/loading, in kb of data");
  declareProperty("NumChunks", 10, "Number of chunks to load or write");
  declareProperty("Compress", true, "For writing: compress the data.");
  declareProperty("HDFCacheSize", 2000000, "HDF cache size, in bytes");
  declareProperty(
      "ClearDiskCache", false,
      "Clear the linux disk cache before loading.\n"
      "Only works on linux AND you need to run MantidPlot in sudo mode (!).");

  std::vector<std::string> types;
  types.push_back("Zeros");
  types.push_back("Incrementing Numbers");
  types.push_back("Random Numbers");
  declareProperty("FakeData", "Incrementing Numbers",
                  boost::make_shared<StringListValidator>(types),
                  "For writing: type of fake data to generate.");

  declareProperty(
      "CompressionFactor", 0.0,
      "The size of the file divided by the the size of the data on disk.",
      Direction::Output);
  declareProperty("SaveSpeed", 0.0,
                  "The measured rate of saving the file, in MB (of data)/sec.",
                  Direction::Output);
  declareProperty("LoadSpeed", 0.0,
                  "The measured rate of loading the file, in MB (of data)/sec.",
                  Direction::Output);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void NexusTester::exec() {
  std::string SaveFilename = getPropertyValue("SaveFilename");
  std::string LoadFilename = getPropertyValue("LoadFilename");
  std::string FakeDataType = getPropertyValue("FakeData");
  int ChunkSizeKb = getProperty("ChunkSize");
  int NumChunks = getProperty("NumChunks");
  bool Compress = getProperty("Compress");

  if (ChunkSizeKb <= 0)
    throw std::invalid_argument("ChunkSize must be > 0");
  if (NumChunks <= 0)
    throw std::invalid_argument("NumChunks must be > 0");

  // Size of the chunk in number of integers
  size_t chunkSize = ChunkSizeKb * 1024 / sizeof(uint32_t);
  // ----------- Generate the fake data -----------------------------
  uint32_t *fakeData = new uint32_t[chunkSize];
  if (FakeDataType == "Zeros") {
    for (size_t i = 0; i < chunkSize; i++)
      fakeData[i] = 0;
  } else if (FakeDataType == "Incrementing Numbers") {
    for (size_t i = 0; i < chunkSize; i++)
      fakeData[i] = uint32_t(i);
  } else if (FakeDataType == "Random Numbers") {
    for (size_t i = 0; i < chunkSize; i++)
      fakeData[i] = rand();
  }

  std::vector<int64_t> dims;
  dims.push_back(int64_t(chunkSize) * NumChunks);
  std::vector<int64_t> chunkDims;
  chunkDims.push_back(int64_t(chunkSize));

  // Total size in BYTES
  double dataSizeMB =
      double(chunkSize * NumChunks * sizeof(uint32_t)) / (1024. * 1024.);
  g_log.notice() << "Data size is " << dataSizeMB << " MB" << std::endl;

  // ------------------------ Save a File ----------------------------
  if (!SaveFilename.empty()) {
    ::NeXus::File file(SaveFilename, NXACC_CREATE5);
    file.makeGroup("FakeDataGroup", "NXdata", true);
    file.makeCompData("FakeData", ::NeXus::UINT32, dims,
                      Compress ? ::NeXus::LZW : ::NeXus::NONE, chunkDims, true);
    Progress prog(this, 0.0, 1.0, NumChunks);
    CPUTimer tim;
    for (int i = 0; i < NumChunks; i++) {
      std::vector<int64_t> startDims;
      startDims.push_back(i * int64_t(chunkSize));
      file.putSlab(fakeData, startDims, chunkDims);
      prog.report();
    }
    file.close();
    double seconds = tim.elapsedWallClock(false);
    double MBperSec = dataSizeMB / seconds;
    g_log.notice() << tim << " to save the file = " << MBperSec << " MB/sec"
                   << std::endl;
    this->setProperty("SaveSpeed", MBperSec);
  }

  // Check the size of the file created/loaded
  Poco::File info(SaveFilename.empty() ? LoadFilename : SaveFilename);
  double fileSizeMB = double(info.getSize()) / (1024. * 1024.);
  g_log.notice() << "File size is " << fileSizeMB << " MB" << std::endl;

  double CompressionFactor = fileSizeMB / dataSizeMB;
  this->setProperty("CompressionFactor", CompressionFactor);

  bool ClearDiskCache = this->getProperty("ClearDiskCache");
  if (ClearDiskCache) {
    g_log.information() << "Clearing disk cache." << std::endl;
    if (system("sync ; echo 3 > /proc/sys/vm/drop_caches") != 0)
      g_log.error("Error clearing disk cache");
    Poco::Thread::sleep(100);
  }

  // ------------------------ Load a File ----------------------------
  if (!LoadFilename.empty()) {
    ::NeXus::File file(LoadFilename, NXACC_READ);
    int HDFCacheSize = getProperty("HDFCacheSize");
    NXsetcache(HDFCacheSize);
    file.openGroup("FakeDataGroup", "NXdata");
    Progress prog(this, 0.0, 1.0, NumChunks);
    CPUTimer tim;

    for (int i = 0; i < NumChunks; i++) {
      file.openData("FakeData");
      std::vector<int64_t> startDims;
      startDims.push_back(i * int64_t(chunkSize));
      file.getSlab(fakeData, startDims, chunkDims);
      prog.report();
      file.closeData();
    }

    file.close();

    double seconds = tim.elapsedWallClock(false);
    double MBperSec = dataSizeMB / seconds;
    this->setProperty("LoadSpeed", MBperSec);
    g_log.notice() << tim << " to load the file = " << MBperSec
                   << " MB/sec (data), " << MBperSec * CompressionFactor
                   << " MB/sec (file)" << std::endl;
  }

  delete[] fakeData;
}

} // namespace Mantid
} // namespace DataHandling
