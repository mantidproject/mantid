/*WIKI*

'''This algorithm is meant for developers only!'''

This algorithm is used for performance testing and debugging of nexus saving and loading.

If you specify SaveFilename (optional), then the algorithm will save a file with the
given number of chunks of the given size.

If you specify LoadFilename (optional), then the algorithm will load back a file
created with this algorithm.

The ''SaveSpeed'' and ''LoadSpeed'' output properties are set to the
saving and loading rates, in MB per second.

*WIKI*/

#include "MantidDataHandling/NexusTester.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidNexusCPP/NeXusFile.hpp"
#include <stdlib.h>
#include "MantidKernel/CPUTimer.h"
#include "MantidAPI/Progress.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace DataHandling
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(NexusTester)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  NexusTester::NexusTester()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  NexusTester::~NexusTester()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string NexusTester::name() const { return "NexusTester";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int NexusTester::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string NexusTester::category() const { return "DataHandling\\NeXus";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void NexusTester::initDocs()
  {
    this->setWikiSummary("Algorithm for testing and debugging purposes only!");
    this->setOptionalMessage("Algorithm for testing and debugging purposes only!");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void NexusTester::init()
  {
    std::vector<std::string> exts;
    exts.push_back(".nxs");
    declareProperty(new FileProperty("SaveFilename", "", FileProperty::OptionalSave, exts),
          "The name of the Nexus file to write");

    declareProperty(new FileProperty("LoadFilename", "", FileProperty::OptionalLoad, exts),
          "The name of the Nexus file to write");

    declareProperty("ChunkSize", 10, "Chunk size for writing/loading, in kb");
    declareProperty("NumChunks", 10, "Number of chunks to load or write");
    declareProperty("Compress", true, "For writing: compress the data.");

    std::vector<std::string> types;
    types.push_back("Zeros");
    types.push_back("Incrementing Numbers");
    types.push_back("Random Numbers");
    declareProperty("FakeData", "Incrementing Numbers", boost::make_shared<StringListValidator>(types) ,
        "For writing: type of fake data to generate.");

    declareProperty("SaveSpeed", 0.0, Direction::Output);
    declareProperty("LoadSpeed", 0.0, Direction::Output);
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void NexusTester::exec()
  {
    std::string SaveFilename = getPropertyValue("SaveFilename");
    std::string LoadFilename = getPropertyValue("LoadFilename");
    std::string FakeDataType = getPropertyValue("FakeData");
    int ChunkSizeKb = getProperty("ChunkSize");
    int NumChunks = getProperty("NumChunks");
    bool Compress = getProperty("Compress");

    if (ChunkSizeKb <= 0) throw std::invalid_argument("ChunkSize must be > 0");
    if (NumChunks <= 0) throw std::invalid_argument("NumChunks must be > 0");

    size_t chunkSize = ChunkSizeKb*1024;
    // ----------- Generate the fake data -----------------------------
    uint32_t * fakeData = new uint32_t[chunkSize];
    if (FakeDataType == "Zeros")
    {
      for (size_t i=0; i<chunkSize; i++)
        fakeData[i] = 0;
    } else if (FakeDataType == "Incrementing Numbers")
    {
      for (size_t i=0; i<chunkSize; i++)
        fakeData[i] = uint32_t(i);
    } else if (FakeDataType == "Random Numbers")
    {
      for (size_t i=0; i<chunkSize; i++)
        fakeData[i] = rand();
    }

    std::vector<int> dims;
    dims.push_back(int(chunkSize)*NumChunks);
    std::vector<int> chunkDims;
    chunkDims.push_back(int(chunkSize));

    size_t totalSize = int(chunkSize)*NumChunks;

    // ------------------------ Save a File ----------------------------
    if (!SaveFilename.empty())
    {
      ::NeXus::File file(SaveFilename, NXACC_CREATE5);
      file.makeGroup("FakeDataGroup", "NXdata", true);
      file.makeCompData("FakeData", ::NeXus::UINT32, dims, Compress ? ::NeXus::LZW : ::NeXus::NONE, chunkDims, true);
      Progress prog(this, 0.0, 1.0, NumChunks);
      CPUTimer tim;
      for (int i=0; i<NumChunks; i++)
      {
        std::vector<int> startDims;
        startDims.push_back(i * int(chunkSize));
        file.putSlab(fakeData, startDims, chunkDims);
        prog.report();
      }
      file.close();
      double seconds = tim.elapsed(false);
      double MBperSec = double(totalSize)/(1024.*1024.*seconds);
      this->setProperty("SaveSpeed", MBperSec);
      g_log.notice() << tim << " to save the file = " << MBperSec << " MB/sec" << std::endl;
    }

    if (!LoadFilename.empty())
    {
      ::NeXus::File file(LoadFilename, NXACC_READ);
      file.openGroup("FakeDataGroup", "NXdata");
      file.openData("FakeData");
      Progress prog(this, 0.0, 1.0, NumChunks);
      CPUTimer tim;

      for (int i=0; i<NumChunks; i++)
      {
        std::vector<int> startDims;
        startDims.push_back(i * int(chunkSize));
        file.getSlab(fakeData, startDims, chunkDims);
        prog.report();
      }
      file.close();

      double seconds = tim.elapsed(false);
      double MBperSec = double(totalSize)/(1024.*1024.*seconds);
      this->setProperty("LoadSpeed", MBperSec);
      g_log.notice() << tim << " to load the file = " << MBperSec << " MB/sec" << std::endl;
    }

  }



} // namespace Mantid
} // namespace DataHandling
