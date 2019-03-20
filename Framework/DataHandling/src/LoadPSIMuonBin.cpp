// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadPSIMuonBin.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/BinaryStreamReader.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/UnitLabelTypes.h"

#include <boost/shared_ptr.hpp>
#include <fstream>
#include <map>

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
  Mantid::Kernel::BinaryStreamReader streamReader(stream);
  std::string fileFormat;
  streamReader.read(fileFormat, 2);
  if (fileFormat != "1N") {
    return 0;
  }
  return 90;
}

// version 1 however there is an issue open to create a version which
// handles temperatures aswell
bool LoadPSIMuonBin::loadMutipleAsOne() { return false; }

void LoadPSIMuonBin::init() {
  const std::vector<std::string> exts{".bin"};
  declareProperty(Kernel::make_unique<Mantid::API::FileProperty>(
                      "Filename", "", Mantid::API::FileProperty::Load, exts),
                  "The name of the Bin file to load");

  declareProperty(Kernel::make_unique<
                      Mantid::API::WorkspaceProperty<Mantid::API::Workspace>>(
                      "OutputWorkspace", "", Kernel::Direction::Output),
                  "An output workspace.");
}

void LoadPSIMuonBin::exec() {

  if (sizeof(float) != 4) {
    // assumpetions made about the binary input won't work but since there is no
    // way to guarantee floating points are 32 bits on all Operating systems
    // here we are.
    throw std::runtime_error("A float on this machine is not 32 bits");
  }

  std::string binFilename = getPropertyValue("Filename");

  std::ifstream binFile(binFilename, std::ios::in | std::ios::binary);

  Mantid::Kernel::BinaryStreamReader streamReader(binFile);
  // Read the first two bytes into a string
  std::string fileFormat;
  streamReader.read(fileFormat, 2);
  if (fileFormat != "1N") {
    throw std::runtime_error(
        "Loaded file is not of PSIMuonBin type (First 2 bytes != 1N)");
  }

  readInHeader(streamReader);
  readInHistograms(streamReader);

  binFile.close();

  // Create the workspace stuff
  generateUnknownAxis();

  auto sizeOfXForHistograms = m_histograms[0].size() + 1;
  DataObjects::Workspace2D_sptr outputWorkspace =
      DataObjects::create<DataObjects::Workspace2D>(
          m_header.numberOfHistograms,
          Mantid::HistogramData::Histogram(
              Mantid::HistogramData::BinEdges(sizeOfXForHistograms)));

  for (auto specNum = 0u; specNum < m_histograms.size(); ++specNum) {
    outputWorkspace->mutableX(specNum) = m_xAxis;
    outputWorkspace->mutableY(specNum) = m_histograms[specNum];
    outputWorkspace->mutableE(specNum) = m_eAxis[specNum];
  }

  assignOutputWorkspaceParticulars(outputWorkspace);

  setProperty("OutputWorkspace", outputWorkspace);
}

std::string LoadPSIMuonBin::getFormattedDateTime(std::string date,
                                                 std::string time) {
  std::map<std::string, std::string> months{
      {"JAN", "01"}, {"FEB", "02"}, {"MAR", "03"}, {"APR", "04"},
      {"MAY", "05"}, {"JUN", "06"}, {"JUL", "07"}, {"AUG", "08"},
      {"SEP", "09"}, {"OCT", "10"}, {"NOV", "11"}, {"DEC", "12"}};
  return "20" + date.substr(7, 2) + "-" +
         months.find(date.substr(3, 3))->second + "-" + date.substr(0, 2) +
         "T" + time;
}

void LoadPSIMuonBin::readSingleVariables(
    Mantid::Kernel::BinaryStreamReader &streamReader) {
  // The single variables in the header of the binary file:
  // Should be at 3rd byte
  streamReader >> m_header.tdcResolution;

  // Should be at 5th byte
  streamReader >> m_header.tdcOverflow;

  // Should be at 7th byte
  streamReader >> m_header.numberOfRuns;

  // This may be 29 but set to 28
  streamReader.moveStreamToPosition(28);
  streamReader >> m_header.lengthOfHistograms;

  // Should be at 31st byte
  streamReader >> m_header.numberOfHistograms;

  streamReader.moveStreamToPosition(424);
  streamReader >> m_header.totalEvents;

  streamReader.moveStreamToPosition(1012);
  streamReader >> m_header.histogramBinWidth;

  if (m_header.histogramBinWidth == 0) {
    m_header.histogramBinWidth =
        static_cast<float>((625.E-6) / 8. *
                           pow(static_cast<float>(2.),
                               static_cast<float>(m_header.tdcResolution)));
  }

  streamReader.moveStreamToPosition(712);
  streamReader >> m_header.monNumberOfevents;

  streamReader.moveStreamToPosition(128); // numdef
  streamReader >> m_header.numberOfDataRecordsFile;

  // Should be at 130th byte
  streamReader.moveStreamToPosition(130); // lendef
  streamReader >> m_header.lengthOfDataRecordsBin;

  // Should be at 132nd byte
  streamReader.moveStreamToPosition(132); // kdafhi
  streamReader >> m_header.numberOfDataRecordsHistogram;

  // Should be at 134th Byte
  streamReader.moveStreamToPosition(134); // khidaf
  streamReader >> m_header.numberOfHistogramsPerRecord;

  streamReader.moveStreamToPosition(654);
  streamReader >> m_header.periodOfSave;

  // Should be at 658th byte
  streamReader >> m_header.periodOfMon;
}

void LoadPSIMuonBin::readStringVariables(
    Mantid::Kernel::BinaryStreamReader &streamReader) {
  // The strings in the header of the binary file:
  streamReader.moveStreamToPosition(138);
  // Only pass 10 bytes into the string from stream
  streamReader.read(m_header.sample, 10);

  streamReader.moveStreamToPosition(148);
  // Only pass 10 bytes into the string from stream
  streamReader.read(m_header.temp, 10);

  streamReader.moveStreamToPosition(158);
  // Only pass 10 bytes into the string from stream
  streamReader.read(m_header.field, 10);

  streamReader.moveStreamToPosition(168);
  // Only pass 10 bytes into the string from stream
  streamReader.read(m_header.orientation, 10);

  streamReader.moveStreamToPosition(860);
  // Only pass 62 bytes into the string from stream
  streamReader.read(m_header.comment, 62);

  streamReader.moveStreamToPosition(218);
  // Only pass 9 bytes into the string from stream
  streamReader.read(m_header.dateStart, 9);

  streamReader.moveStreamToPosition(227);
  // Only pass 9 bytes into the string from stream
  streamReader.read(m_header.dateEnd, 9);

  streamReader.moveStreamToPosition(236);
  // Only pass 8 bytes into the string from stream
  streamReader.read(m_header.timeStart, 8);

  streamReader.moveStreamToPosition(244);
  // Only pass 8 bytes into the string from stream
  streamReader.read(m_header.timeEnd, 8);

  streamReader.moveStreamToPosition(60);
  // Only pass 11 bytes into the string from stream
  streamReader.read(m_header.monDeviation, 11);
}

void LoadPSIMuonBin::readArrayVariables(
    Mantid::Kernel::BinaryStreamReader &streamReader) {
  // The arrays in the header of the binary file:
  for (auto i = 0u; i <= 5; ++i) {
    streamReader.moveStreamToPosition(924 + (i * 4));
    streamReader.read(m_header.labels_scalars[i], 4);

    streamReader.moveStreamToPosition(670 + (i * 4));
    streamReader >> m_header.scalars[i];
  }

  for (auto i = 6u; i < 18; ++i) {
    streamReader.moveStreamToPosition(554 + ((i - 6) * 4));
    streamReader.read(m_header.labels_scalars[i], 4);

    streamReader.moveStreamToPosition(360 + ((i - 6) * 4));
    streamReader >> m_header.scalars[i];
  }

  for (auto i = 0u; i <= 15; ++i) {
    streamReader.moveStreamToPosition(948 + (i * 4));
    streamReader >> m_header.labelsOfHistograms[i];

    streamReader.moveStreamToPosition(458 + (i * 2));
    streamReader >> m_header.integerT0[i];

    streamReader.moveStreamToPosition(490 + (i * 2));
    streamReader >> m_header.firstGood[i];

    streamReader.moveStreamToPosition(522 + (i * 2));
    streamReader >> m_header.lastGood[i];

    streamReader.moveStreamToPosition(792 + (i * 4));
    streamReader >> m_header.realT0[i];
  }

  for (auto i = 0u; i < 4; ++i) {
    streamReader.moveStreamToPosition(716 + (i * 4));
    streamReader >> m_header.temperatures[i];

    streamReader.moveStreamToPosition(738 + (i * 4));
    streamReader >> m_header.temperatureDeviation[i];

    streamReader.moveStreamToPosition(72 + (i * 4));
    streamReader >> m_header.monLow[i];

    streamReader.moveStreamToPosition(88 + (i * 4));
    streamReader >> m_header.monHigh[i];
  }
}

void LoadPSIMuonBin::readInHeader(
    Mantid::Kernel::BinaryStreamReader &streamReader) {
  readSingleVariables(streamReader);
  readStringVariables(streamReader);
  readArrayVariables(streamReader);
}

void LoadPSIMuonBin::readInHistograms(
    Mantid::Kernel::BinaryStreamReader &streamReader) {
  // Read in the m_histograms
  for (auto histogramIndex = 0; histogramIndex < m_header.numberOfHistograms;
       ++histogramIndex) {
    std::vector<double> nextHistogram;
    for (auto rowIndex = 0; rowIndex < m_header.lengthOfHistograms;
         ++rowIndex) {
      // Each histogram bit is 1024 bytes below the file start, and 4 bytes
      // apart, and the HistogramNumber * NumberOfRecordsInEach *
      // LengthOfTheDataRecordBins + PositionInHistogram
      unsigned long histogramStreamPosition =
          1024 + (histogramIndex * m_header.numberOfDataRecordsFile *
                  m_header.lengthOfDataRecordsBin);
      unsigned long streamPosition =
          histogramStreamPosition + rowIndex * sizeof(int32_t);
      streamReader.moveStreamToPosition(streamPosition);
      int32_t nextReadValue;
      streamReader >> nextReadValue;
      nextHistogram.push_back(nextReadValue);
    }
    m_histograms.push_back(nextHistogram);
  }
}

void LoadPSIMuonBin::generateUnknownAxis() {
  // Create a x axis, assumption that m_histograms will all be the same size,
  // and that x will be 1 more in size than y
  for (auto xIndex = 0;
       static_cast<unsigned int>(xIndex) < m_histograms[0].size() + 1;
       ++xIndex) {
    m_xAxis.push_back(static_cast<double>(xIndex) * m_header.histogramBinWidth);
  }

  // Create Errors
  for (auto &histogram : m_histograms) {
    std::vector<double> newEAxis;
    for (auto eIndex = 0u; eIndex < m_histograms[0].size(); ++eIndex) {
      newEAxis.push_back(sqrt(histogram[eIndex]));
    }
    m_eAxis.push_back(newEAxis);
  }
}

void LoadPSIMuonBin::assignOutputWorkspaceParticulars(
    DataObjects::Workspace2D_sptr &outputWorkspace) {
  // Sort some workspace particulars
  outputWorkspace->setTitle(m_header.sample +
                            " - Run:" + std::to_string(m_header.numberOfRuns));

  // Set the detector IDs
  for (auto i = 0; i < m_header.numberOfHistograms; ++i) {
    outputWorkspace->getSpectrum(i).setDetectorID(i);
  }

  // Set Run Property goodfrm
  outputWorkspace->mutableRun().addProperty("goodfrm", 1);

  // Set axis variables
  outputWorkspace->setYUnit("Counts");
  boost::shared_ptr<Kernel::Units::Label> lblUnit =
      boost::dynamic_pointer_cast<Kernel::Units::Label>(
          Kernel::UnitFactory::Instance().create("Label"));
  lblUnit->setLabel("Time", Kernel::Units::Symbol::Microsecond);
  outputWorkspace->getAxis(0)->unit() = lblUnit;

  // Set Start date and time and end date and time
  Mantid::Types::Core::DateAndTime start(
      getFormattedDateTime(m_header.dateStart, m_header.timeStart));
  Mantid::Types::Core::DateAndTime end(
      getFormattedDateTime(m_header.dateEnd, m_header.timeEnd));
  outputWorkspace->mutableRun().setStartAndEndTime(start, end);

  // Start the log building
  Mantid::API::Algorithm_sptr logAlg = createChildAlgorithm("AddSampleLog");
  logAlg->setProperty("Workspace", outputWorkspace);

  // Log the set temperature
  logAlg->setProperty("LogType", "String");
  logAlg->setProperty("LogName", "Set Temperature");
  logAlg->setProperty("LogText", m_header.temp);
  logAlg->executeAsChildAlg();

  // Add The other temperatures as log
  auto sizeOfTemps =
      sizeof(m_header.temperatures) / sizeof(*m_header.temperatures);
  for (auto tempNum = 1u; tempNum < sizeOfTemps + 1; ++tempNum) {
    if (m_header.temperatures[tempNum - 1] != 0) {
      logAlg->setProperty("LogType", "String");
      logAlg->setProperty("LogName",
                          "Actual Temperature" + std::to_string(tempNum));
      logAlg->setProperty("LogText",
                          std::to_string(m_header.temperatures[tempNum - 1]));
      logAlg->executeAsChildAlg();

      // Temperature deviation
      logAlg->setProperty("LogType", "String");
      logAlg->setProperty("LogName",
                          "Temperature Deviation" + std::to_string(tempNum));
      logAlg->setProperty(
          "LogText",
          std::to_string(m_header.temperatureDeviation[tempNum - 1]));
      logAlg->executeAsChildAlg();
    }
  }

  // Set the comment
  outputWorkspace->setComment(m_header.comment);
  logAlg->setProperty("LogType", "String");
  logAlg->setProperty("LogName", "Comment");
  logAlg->setProperty("LogText", m_header.comment);
  logAlg->executeAsChildAlg();

  // Length of run
  logAlg->setProperty("LogType", "String");
  logAlg->setProperty("LogName", "Length of Run");
  logAlg->setProperty(
      "LogText", std::to_string((static_cast<double>(m_histograms[0].size())) *
                                m_header.histogramBinWidth) +
                     "MicroSeconds");
  logAlg->executeAsChildAlg();

  // Log field
  logAlg->setProperty("LogType", "String");
  logAlg->setProperty("LogName", "Field");
  logAlg->setProperty("LogText", m_header.field);
  logAlg->executeAsChildAlg();

  // get scalar labels and set spectra accordingly
  for (auto i = 0u; i < sizeof(m_header.scalars) / sizeof(*m_header.scalars);
       ++i) {
    if (m_header.labels_scalars[i] != "NONE") {
      logAlg->setProperty("LogType", "String");
      logAlg->setProperty("LogName",
                          "Spectra" + std::to_string(i) + " label and scalar");
      logAlg->setProperty("LogText", m_header.labels_scalars[i] + " - " +
                                         std::to_string(m_header.scalars[i]));
      logAlg->executeAsChildAlg();
    } else {
      break;
    }
  }

  // Orientation
  logAlg->setProperty("LogType", "String");
  logAlg->setProperty("LogName", "Orientation");
  logAlg->setProperty("LogText", m_header.orientation);
  logAlg->executeAsChildAlg();

  // first good and last good
  for (size_t i = 0;
       i < sizeof(m_header.firstGood) / sizeof(*m_header.firstGood); ++i) {
    if (m_header.firstGood[i] != 0) {
      logAlg->setProperty("LogType", "String");
      logAlg->setProperty("LogName", "First and Last good for Spectra #" +
                                         std::to_string(i));
      logAlg->setProperty("LogText", std::to_string(m_header.firstGood[i]) +
                                         " - " +
                                         std::to_string(m_header.lastGood[i]));
      logAlg->executeAsChildAlg();
    }
  }

  // total events
  logAlg->setProperty("LogType", "String");
  logAlg->setProperty("LogName", "Total Number of Events");
  logAlg->setProperty("LogText", std::to_string(m_header.totalEvents));
  logAlg->executeAsChildAlg();

  // tdcResolution
  logAlg->setProperty("LogType", "String");
  logAlg->setProperty("LogName", "TDC Resolution");
  logAlg->setProperty("LogText", std::to_string(m_header.tdcResolution));
  logAlg->executeAsChildAlg();

  // tdcOverflow
  logAlg->setProperty("LogType", "String");
  logAlg->setProperty("LogName", "TDC Overflow");
  logAlg->setProperty("LogText", std::to_string(m_header.tdcOverflow));
  logAlg->executeAsChildAlg();

  // Length of spectra
  logAlg->setProperty("LogType", "String");
  logAlg->setProperty("LogName", "Spectra Length");
  logAlg->setProperty("LogText", std::to_string(m_header.lengthOfHistograms));
  logAlg->executeAsChildAlg();

  // Number of Spectra
  logAlg->setProperty("LogType", "String");
  logAlg->setProperty("LogName", "Number of Spectra");
  logAlg->setProperty("LogText", std::to_string(m_header.numberOfHistograms));
  logAlg->executeAsChildAlg();

  // monNumber of events
  logAlg->setProperty("LogType", "String");
  logAlg->setProperty("LogName", "Mon number of events");
  logAlg->setProperty("LogText", std::to_string(m_header.monNumberOfevents));
  logAlg->executeAsChildAlg();

  // Mon Period
  logAlg->setProperty("LogType", "String");
  logAlg->setProperty("LogName", "Mon Period");
  logAlg->setProperty("LogText", std::to_string(m_header.periodOfMon));
  logAlg->executeAsChildAlg();

  if (m_header.monLow[0] == 0 && m_header.monHigh[0] == 0) {
    logAlg->setProperty("LogType", "String");
    logAlg->setProperty("LogName", "Mon Low");
    logAlg->setProperty("LogText", "0");
    logAlg->executeAsChildAlg();

    logAlg->setProperty("LogType", "String");
    logAlg->setProperty("LogName", "Mon High");
    logAlg->setProperty("LogText", "0");
    logAlg->executeAsChildAlg();
  } else {
    for (auto i = 0u; i < 4; ++i) {
      if (m_header.monLow[i] == 0 || m_header.monHigh[i] == 0)
        break;
      logAlg->setProperty("LogType", "String");
      logAlg->setProperty("LogName", "Mon Low" + std::to_string(i));
      logAlg->setProperty("LogText", std::to_string(m_header.monLow[i]));
      logAlg->executeAsChildAlg();

      logAlg->setProperty("LogType", "String");
      logAlg->setProperty("LogName", "Mon High + i");
      logAlg->setProperty("LogText", std::to_string(m_header.monHigh[i]));
      logAlg->executeAsChildAlg();
    }
  }

  // Mon Deviation
  logAlg->setProperty("LogType", "String");
  logAlg->setProperty("LogName", "Mon Deviation");
  logAlg->setProperty("LogText", m_header.monDeviation);
  logAlg->executeAsChildAlg();

  if (m_header.realT0[0] != 0) {
    for (const float &i : m_header.realT0) {
      if (i == 0)
        break;
      logAlg->setProperty("LogType", "String");
      logAlg->setProperty("LogName", "realT0 + i");
      logAlg->setProperty("LogText", std::to_string(i));
      logAlg->executeAsChildAlg();
    }
  }
}

} // namespace DataHandling
} // namespace Mantid
