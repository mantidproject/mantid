#include "MantidDataHandling/LoadPSIMuonBin.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidKernel/BinaryStreamReader.h"

#include <fstream>

namespace Mantid {
namespace DataHandling {

DECLARE_FILELOADER_ALGORITHM(LoadPSIMuonBin)

// Algorithm's name for identification. @see Algorithm::name
const std::string LoadPSIMuonBin::name() const { return "LoadPSIMuonBin"; }

// Algorithm's version for identification. @see Algorithm::version
int LoadPSIMuonBin::version() const { return 1; }

const std::string LoadPSIMuonBin::summary() const {
  return "Loads a data file that is in PSI Muon Binary format into a "
         "workspace (Workspace2D class).";
}

// Algorithm's category for identification. @see Algorithm::category
const std::string LoadPSIMuonBin::category() const {
  return "DataHandling\\PSIMuonBin";
}

int LoadPSIMuonBin::confidence(Kernel::FileDescriptor &descriptor) const {
  auto &stream = descriptor.data();
  // 85th character is a space & 89th character is a ~
  stream.seekg(0, std::ios::beg);
  int c = stream.get();
  int confidence(0);
  // Checks if postion 0 is a 1
  if (c == 49) {
    stream.seekg(1, std::ios::cur);
    int c = stream.get();
    // Checks if position 1 is N
    if (c == 78)
      confidence = 90;
  }
  return confidence;
}

// version 1 does not however there is an issue open to create a version which
// handles temperatures aswell
bool LoadPSIMuonBin::loadMutipleAsOne() { return false; }

void LoadPSIMuonBin::init() {
  const std::vector<std::string> exts{".bin"};
  declareProperty(Kernel::make_unique<Mantid::API::FileProperty>("Filename", "",
                                                    Mantid::API::FileProperty::Load, exts),
                  "The name of the Bin file to load");

  declareProperty(Kernel::make_unique<Mantid::API::WorkspaceProperty<Mantid::API::Workspace>>(
                      "OutputWorkspace", "", Kernel::Direction::Output),
                  "An output workspace.");
}

void LoadPSIMuonBin::exec() {

  if (sizeof(float) != 4) {
    // assumpetions made about the binary input won't work but since there is no
    // way to guarantee floating points are 32 bits on all Operating systems
    // here we are.
  }

  std::string binFilename = getPropertyValue("Filename");
  std::vector<Mantid::HistogramData::Histogram> readHistogramData;

  std::ifstream binFile(binFilename, std::ios::in | std::ios::binary | std::ios::ate);

  Mantid::Kernel::BinaryStreamReader streamReader(binFile);
  //Read the first two bytes into a string
  std::string fileFormat;
  streamReader.read(fileFormat, 2);
  if (fileFormat != "1N") {
    // Throw Error for file type
  }

  //The single variables in the header of the binary file:
  //Should be at 3rd byte
  int16_t tdcResolution;
  streamReader >> tdcResolution;
  //Should be at 5th byte
  int16_t tdcOverflow;
  streamReader >> tdcOverflow;
  //Should be at 7th byte
  int16_t numberOfRuns;
  streamReader >> numberOfRuns;

  streamReader.moveStreamToPosition(29);
  int16_t lengthOfHistograms;
  streamReader >> lengthOfHistograms;
  //Should be at 31st byte
  int16_t numberOfHistograms;
  streamReader >> numberOfHistograms;

  streamReader.moveStreamToPosition(424);
  int32_t totalEvents;
  streamReader >> totalEvents;

  streamReader.moveStreamToPosition(1012);
  float histogramBinWidth;
  streamReader >> histogramBinWidth;

  if(histogramBinWidth == 0){
    histogramBinWidth = (625.E-6)/8.*pow(static_cast<float>(2.), static_cast<float>(tdcResolution));
  }

  
  int32_t monNumberOfevents;
  int16_t numberOfDataRecordsFile;      // numdef
  int16_t lengthOfDataRecordsBin;       // lendef
  int16_t numberOfDataRecordsHistogram; // kdafhi
  int16_t numberOfHistogramsPerRecord;  // khidaf
  int32_t periodOfSave;
  int32_t periodOfMon;

  //The strings in the header of the binary file:
  std::string sample; // Only pass 10 bytes into the string from stream
  std::string temp; // Only pass 10 bytes into the string from stream
  std::string field; // Only pass 10 bytes into the string from stream
  std::string orientation; // Only pass 10 bytes into the string from stream
  std::string comment; // Only pass 62 bytes into the string from stream
  std::string dateStart; // Only pass 9 bytes into the string from stream
  std::string dateEnd; // Only pass 9 bytes into the string from stream
  std::string timeStart; // Only pass 8 bytes into the string from stream
  std::string timeEnd; // Only pass 8 bytes into the string from stream
  std::string monDeviation; // Only pass 11 bytes into the string from stream
  std::string labels_scalars[18]; // 18 Strings with 4 max length

  //The arrays in the header of the binary file:
  int32_t labelsOfHistograms[16];
  int16_t integerT0[16];
  int16_t firstGood[16];
  int16_t lastGood[16];
  float real_t0[16];
  int32_t scalars[18];
  float temperatures[4];
  float temperatureDeviation[4];
  float monLow[4];
  float monHigh[4];
}
} // namespace DataHandling
} // namespace Mantid
