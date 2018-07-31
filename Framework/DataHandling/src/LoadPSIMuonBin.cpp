#include "MantidDataHandling/LoadPSIMuonBin.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidKernel/BinaryStreamReader.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidDataObjects/Workspace2D.h"

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

  std::ifstream binFile(binFilename, std::ios::in | std::ios::binary);

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

  //This may be 29 but set to 28
  streamReader.moveStreamToPosition(28);
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
    histogramBinWidth = static_cast<float>((625.E-6)/8.*pow(static_cast<float>(2.), static_cast<float>(tdcResolution)));
  }

  streamReader.moveStreamToPosition(712);
  int32_t monNumberOfevents;
  streamReader >> monNumberOfevents;

  streamReader.moveStreamToPosition(128);
  int16_t numberOfDataRecordsFile;      // numdef
  streamReader >> numberOfDataRecordsFile;

  //Should be at 130th byte
  streamReader.moveStreamToPosition(130);
  int16_t lengthOfDataRecordsBin;       // lendef
  streamReader >> lengthOfDataRecordsBin;

  //Should be at 132nd byte
  streamReader.moveStreamToPosition(132);
  int16_t numberOfDataRecordsHistogram; // kdafhi
  streamReader >> numberOfDataRecordsHistogram;

  //Should be at 134th Byte
  streamReader.moveStreamToPosition(134);
  int16_t numberOfHistogramsPerRecord;  // khidaf
  streamReader >> numberOfHistogramsPerRecord;

  streamReader.moveStreamToPosition(654);
  int32_t periodOfSave;
  streamReader >> periodOfSave;

  //Should be at 658th byte
  int32_t periodOfMon;
  streamReader >> periodOfMon;

  //The strings in the header of the binary file:
  streamReader.moveStreamToPosition(138);
  std::string sample; // Only pass 10 bytes into the string from stream
  streamReader.read(sample, 10);

  streamReader.moveStreamToPosition(148);
  std::string temp; // Only pass 10 bytes into the string from stream
  streamReader.read(temp, 10);

  streamReader.moveStreamToPosition(158);
  std::string field; // Only pass 10 bytes into the string from stream
  streamReader.read(field, 10);

  streamReader.moveStreamToPosition(168);
  std::string orientation; // Only pass 10 bytes into the string from stream
  streamReader.read(orientation, 10);

  streamReader.moveStreamToPosition(860);
  std::string comment; // Only pass 62 bytes into the string from stream
  streamReader.read(comment, 62);

  streamReader.moveStreamToPosition(218);
  std::string dateStart; // Only pass 9 bytes into the string from stream
  streamReader.read(dateStart, 9);

  streamReader.moveStreamToPosition(227);
  std::string dateEnd; // Only pass 9 bytes into the string from stream
  streamReader.read(dateEnd, 9);

  streamReader.moveStreamToPosition(236);
  std::string timeStart; // Only pass 8 bytes into the string from stream
  streamReader.read(timeStart, 8);

  streamReader.moveStreamToPosition(244);
  std::string timeEnd; // Only pass 8 bytes into the string from stream
  streamReader.read(timeEnd, 8);

  streamReader.moveStreamToPosition(60);
  std::string monDeviation; // Only pass 11 bytes into the string from stream
  streamReader.read(monDeviation, 11);

  //The arrays in the header of the binary file:
  int32_t scalars[18];
  std::string labels_scalars[18]; // 18 Strings with 4 max length
  for(auto i = 0u; i <= 5; ++i){
    streamReader.moveStreamToPosition(924 + (i*4));
    streamReader.read(labels_scalars[i], 4); 

    streamReader.moveStreamToPosition(670 + (i*4));
    streamReader >> scalars[i];
  }

  for(auto i = 6u; i < 18; ++i){
    streamReader.moveStreamToPosition(554 + ((i-6)*4));
    streamReader.read(labels_scalars[i], 4);

    streamReader.moveStreamToPosition(360 + ((i-6)*4));
    streamReader >> scalars[i];
  }

  int32_t labelsOfHistograms[16];
  int16_t integerT0[16];
  int16_t firstGood[16];
  int16_t lastGood[16];
  float realT0[16];
  for(auto i = 0u; i <=15; ++i){
    streamReader.moveStreamToPosition(948 + (i*4));
    streamReader >> labelsOfHistograms[i];

    streamReader.moveStreamToPosition(458 + (i*2));
    streamReader >> integerT0[i];

    streamReader.moveStreamToPosition(490 + (i*2));
    streamReader >> firstGood[i];

    streamReader.moveStreamToPosition(522 + (i*2));
    streamReader >> lastGood[i];

    streamReader.moveStreamToPosition(792 + (i*4));
    streamReader >> realT0[i];
  }

  float temperatures[4];
  float temperatureDeviation[4];
  float monLow[4];
  float monHigh[4];
  for(auto i = 0u; i < 4; ++i){
    streamReader.moveStreamToPosition(716 + (i*4));
    streamReader >> temperatures[i];

    streamReader.moveStreamToPosition(738 + (i*4));
    streamReader >> temperatureDeviation[i];

    streamReader.moveStreamToPosition(72 + (i*4));
    streamReader >> monLow[i];

    streamReader.moveStreamToPosition(88 + (i*4));
    streamReader >> monHigh[i];
  }

  //Read in the histograms
  std::vector<std::vector<double>> histograms;
  for(auto histogramIndex = 0; histogramIndex < numberOfHistograms; ++histogramIndex){
    std::vector<double> nextHistogram;
    for(auto rowIndex = 0; rowIndex < lengthOfHistograms; ++rowIndex){
      //Each histogram bit is 1024 bytes below the file start, and 4 bytes apart, and the HistogramNumber * NumberOfRecordsInEach * LengthOfTheDataRecordBins + PositionInHistogram
      unsigned long histogramStreamPosition = 1024+(histogramIndex * numberOfDataRecordsFile * lengthOfDataRecordsBin);
      unsigned long streamPosition = histogramStreamPosition + rowIndex*sizeof(int32_t);
      streamReader.moveStreamToPosition(streamPosition);
      int32_t nextReadValue;
      streamReader >> nextReadValue;
      nextHistogram.push_back(nextReadValue);
    }
    histograms.push_back(nextHistogram);
  }

  binFile.close();

  //Create the workspace stuff
  // Create a x axis, assumption that histograms will all be the same size, and that x will be 1 more in size than y
  std::vector<double> xAxis;
  for(auto xIndex = 0; static_cast<unsigned int>(xIndex) < histograms[0].size()+1; ++xIndex){
    xAxis.push_back(static_cast<double>(xIndex) * histogramBinWidth);
  }

  auto sizeOfXForHistograms = histograms[0].size()+1;
  DataObjects::Workspace2D_sptr outputWorkspace = DataObjects::create<DataObjects::Workspace2D>(numberOfHistograms, Mantid::HistogramData::Histogram(Mantid::HistogramData::BinEdges(sizeOfXForHistograms)));

  for (auto specNum = 0u; specNum < histograms.size(); ++specNum){
    outputWorkspace->mutableX(specNum) = xAxis;
    outputWorkspace->mutableY(specNum) = histograms[specNum];
    //Add Errors and Dx if nessercary
  }

  //Set axis variables
  
  //Set log numbers

  setProperty("OutputWorkspace", outputWorkspace);
}

} // namespace DataHandling
} // namespace Mantid
