#include "MantidDataHandling/LoadPSIMuonBin.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidKernel/BinaryStreamReader.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidAPI/Axis.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/UnitLabelTypes.h"

#include <fstream>
#include <map>
#include <boost/shared_ptr.hpp>

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

// version 1 however there is an issue open to create a version which
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
  streamReader >> m_header.tdcResolution;

  //Should be at 5th byte
  streamReader >> m_header.tdcOverflow;

  //Should be at 7th byte
  streamReader >> m_header.numberOfRuns;

  //This may be 29 but set to 28
  streamReader.moveStreamToPosition(28);
  streamReader >> m_header.lengthOfHistograms;

  //Should be at 31st byte
  streamReader >> m_header.numberOfHistograms;

  streamReader.moveStreamToPosition(424);
  streamReader >> m_header.totalEvents;

  streamReader.moveStreamToPosition(1012);
  streamReader >> m_header.histogramBinWidth;

  if(m_header.histogramBinWidth == 0){
    m_header.histogramBinWidth = static_cast<float>((625.E-6)/8.*pow(static_cast<float>(2.), static_cast<float>(m_header.tdcResolution)));
  }

  streamReader.moveStreamToPosition(712);
  streamReader >> m_header.monNumberOfevents;

  streamReader.moveStreamToPosition(128);   // numdef
  streamReader >> m_header.numberOfDataRecordsFile;

  //Should be at 130th byte
  streamReader.moveStreamToPosition(130);      // lendef
  streamReader >> m_header.lengthOfDataRecordsBin;

  //Should be at 132nd byte
  streamReader.moveStreamToPosition(132); // kdafhi
  streamReader >> m_header.numberOfDataRecordsHistogram;

  //Should be at 134th Byte
  streamReader.moveStreamToPosition(134);  // khidaf
  streamReader >> m_header.numberOfHistogramsPerRecord;

  streamReader.moveStreamToPosition(654);
  streamReader >> m_header.periodOfSave;

  //Should be at 658th byte
  streamReader >> m_header.periodOfMon;

  //The strings in the header of the binary file:
  streamReader.moveStreamToPosition(138); // Only pass 10 bytes into the string from stream
  streamReader.read(m_header.sample, 10);

  streamReader.moveStreamToPosition(148); // Only pass 10 bytes into the string from stream
  streamReader.read(m_header.temp, 10);

  streamReader.moveStreamToPosition(158); // Only pass 10 bytes into the string from stream
  streamReader.read(m_header.field, 10);

  streamReader.moveStreamToPosition(168); // Only pass 10 bytes into the string from stream
  streamReader.read(m_header.orientation, 10);

  streamReader.moveStreamToPosition(860); // Only pass 62 bytes into the string from stream
  streamReader.read(m_header.comment, 62);

  streamReader.moveStreamToPosition(218); // Only pass 9 bytes into the string from stream
  streamReader.read(m_header.dateStart, 9);

  streamReader.moveStreamToPosition(227); // Only pass 9 bytes into the string from stream
  streamReader.read(m_header.dateEnd, 9);

  streamReader.moveStreamToPosition(236); // Only pass 8 bytes into the string from stream
  streamReader.read(m_header.timeStart, 8);

  streamReader.moveStreamToPosition(244); // Only pass 8 bytes into the string from stream
  streamReader.read(m_header.timeEnd, 8);

  streamReader.moveStreamToPosition(60); // Only pass 11 bytes into the string from stream
  streamReader.read(m_header.monDeviation, 11);

  //The arrays in the header of the binary file:
  for(auto i = 0u; i <= 5; ++i){
    streamReader.moveStreamToPosition(924 + (i*4));
    streamReader.read(m_header.labels_scalars[i], 4); 

    streamReader.moveStreamToPosition(670 + (i*4));
    streamReader >> m_header.scalars[i];
  }

  for(auto i = 6u; i < 18; ++i){
    streamReader.moveStreamToPosition(554 + ((i-6)*4));
    streamReader.read(m_header.labels_scalars[i], 4);

    streamReader.moveStreamToPosition(360 + ((i-6)*4));
    streamReader >> m_header.scalars[i];
  }

  for(auto i = 0u; i <=15; ++i){
    streamReader.moveStreamToPosition(948 + (i*4));
    streamReader >> m_header.labelsOfHistograms[i];

    streamReader.moveStreamToPosition(458 + (i*2));
    streamReader >> m_header.integerT0[i];

    streamReader.moveStreamToPosition(490 + (i*2));
    streamReader >> m_header.firstGood[i];

    streamReader.moveStreamToPosition(522 + (i*2));
    streamReader >> m_header.lastGood[i];

    streamReader.moveStreamToPosition(792 + (i*4));
    streamReader >> m_header.realT0[i];
  }

  for(auto i = 0u; i < 4; ++i){
    streamReader.moveStreamToPosition(716 + (i*4));
    streamReader >> m_header.temperatures[i];

    streamReader.moveStreamToPosition(738 + (i*4));
    streamReader >> m_header.temperatureDeviation[i];

    streamReader.moveStreamToPosition(72 + (i*4));
    streamReader >> m_header.monLow[i];

    streamReader.moveStreamToPosition(88 + (i*4));
    streamReader >> m_header.monHigh[i];
  }

  //Read in the histograms
  std::vector<std::vector<double>> histograms;
  for(auto histogramIndex = 0; histogramIndex < m_header.numberOfHistograms; ++histogramIndex){
    std::vector<double> nextHistogram;
    for(auto rowIndex = 0; rowIndex < m_header.lengthOfHistograms; ++rowIndex){
      //Each histogram bit is 1024 bytes below the file start, and 4 bytes apart, and the HistogramNumber * NumberOfRecordsInEach * LengthOfTheDataRecordBins + PositionInHistogram
      unsigned long histogramStreamPosition = 1024+(histogramIndex * m_header.numberOfDataRecordsFile * m_header.lengthOfDataRecordsBin);
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
    xAxis.push_back(static_cast<double>(xIndex) * m_header.histogramBinWidth);
  }

  // Create Errors
  std::vector<std::vector<double >> eAxis;
  for(auto histogramIndex = 0u; histogramIndex < histograms.size(); ++histogramIndex){
    std::vector<double> newEAxis;
    for(auto eIndex = 0u; eIndex < histograms[0].size(); ++eIndex){
      newEAxis.push_back(sqrt(histograms[histogramIndex][eIndex]));
    }
    eAxis.push_back(newEAxis);
  }

  auto sizeOfXForHistograms = histograms[0].size()+1;
  DataObjects::Workspace2D_sptr outputWorkspace = DataObjects::create<DataObjects::Workspace2D>(m_header.numberOfHistograms, Mantid::HistogramData::Histogram(Mantid::HistogramData::BinEdges(sizeOfXForHistograms)));

  for (auto specNum = 0u; specNum < histograms.size(); ++specNum){
    outputWorkspace->mutableX(specNum) = xAxis;
    outputWorkspace->mutableY(specNum) = histograms[specNum];
    outputWorkspace->mutableE(specNum) = eAxis[specNum];
  }

  //Sort some workspace particulars
  outputWorkspace->setTitle(m_header.sample + " - Run:" + std::to_string(m_header.numberOfRuns));

  //Set axis variables
  outputWorkspace->setYUnit("Counts");
  boost::shared_ptr<Kernel::Units::Label> lblUnit =
      boost::dynamic_pointer_cast<Kernel::Units::Label>(
          Kernel::UnitFactory::Instance().create("Label"));
  lblUnit->setLabel("Time", Kernel::Units::Symbol::Microsecond);
  outputWorkspace->getAxis(0)->unit() = lblUnit;

  //Set Start date and time and end date and time 
  Mantid::Types::Core::DateAndTime start(getFormattedDateTime(m_header.dateStart, m_header.timeStart));
  Mantid::Types::Core::DateAndTime end(getFormattedDateTime(m_header.dateEnd, m_header.timeEnd));
  outputWorkspace->mutableRun().setStartAndEndTime(start, end);

  //Start the log building
  Mantid::API::Algorithm_sptr logAlg = createChildAlgorithm("AddSampleLog");
  logAlg->setProperty("Workspace", outputWorkspace);
  
  //Log the set temperature
  logAlg->setProperty("LogType", "String");
  logAlg->setProperty("LogName", "Set Temperature");
  logAlg->setProperty("LogText", m_header.temp);
  logAlg->executeAsChildAlg();

  //Add The other temperatures as log
  auto sizeOfTemps = sizeof(m_header.temperatures)/sizeof(*m_header.temperatures);
  for(auto tempNum = 1u; tempNum < sizeOfTemps + 1; ++tempNum){
    if(m_header.temperatures[tempNum-1] != 0){
      logAlg->setProperty("LogType", "Number");
      logAlg->setProperty("LogName", "Actual Temperature" + std::to_string(tempNum));
      logAlg->setProperty("LogText", std::to_string(m_header.temperatures[tempNum-1]));
      logAlg->executeAsChildAlg();
    }
  }

  //Set the comment
  outputWorkspace->setComment(m_header.comment);
  logAlg->setProperty("LogType", "String");
  logAlg->setProperty("LogName", "Comment");
  logAlg->setProperty("LogText", m_header.comment);
  logAlg->executeAsChildAlg();

  //Length of run
  logAlg->setProperty("LogType", "String");
  logAlg->setProperty("LogName", "Length of Run");
  logAlg->setProperty("LogText", std::to_string((static_cast<double>(histograms[0].size())) * m_header.histogramBinWidth) + "μs");
  logAlg->executeAsChildAlg();

  //Log field
  logAlg->setProperty("LogType", "String");
  logAlg->setProperty("LogName", "Field");
  logAlg->setProperty("LogText", m_header.field);
  logAlg->executeAsChildAlg();

  setProperty("OutputWorkspace", outputWorkspace);
}

std::string LoadPSIMuonBin::getFormattedDateTime(std::string date, std::string time){
    std::map<std::string, std::string> months{{"JAN", "01"},{"FEB", "02"},{"MAR", "03"},{"APR", "04"},{"MAY", "05"},{"JUN", "06"},{"JUL", "07"},{"AUG", "08"},{"SEP", "09"},{"OCT", "10"},{"NOV", "11"},{"DEC", "12"}};
    return "20" + date.substr(7,2) + "-" + months.find(date.substr(3, 3))->second + "-" + date.substr(0,2) + "T" + time;
}

void LoadPSIMuonBin::assignOutputWorkspaceParticulars(DataObjects::Workspace2D_sptr &outputWorkspace){}
} // namespace DataHandling
} // namespace Mantid