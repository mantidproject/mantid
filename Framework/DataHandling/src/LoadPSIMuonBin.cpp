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

#include <Poco/Path.h>
#include <Poco/RecursiveDirectoryIterator.h>
#include <boost/algorithm/string.hpp>
#include <boost/shared_ptr.hpp>
#include <fstream>
#include <map>

namespace Mantid {
namespace DataHandling {

namespace {
const std::map<std::string, std::string> months{
    {"JAN", "01"}, {"FEB", "02"}, {"MAR", "03"}, {"APR", "04"},
    {"MAY", "05"}, {"JUN", "06"}, {"JUL", "07"}, {"AUG", "08"},
    {"SEP", "09"}, {"OCT", "10"}, {"NOV", "11"}, {"DEC", "12"}};
const std::vector<std::string> psiMonths{"JAN", "FEB", "MAR", "APR",
                                         "MAY", "JUN", "JUL", "AUG",
                                         "SEP", "OCT", "NOV", "DEC"};
} // namespace

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
  return "DataHandling\\PSI";
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
  declareProperty(std::make_unique<Mantid::API::FileProperty>(
                      "Filename", "", Mantid::API::FileProperty::Load, exts),
                  "The name of the Bin file to load");

  const std::vector<std::string> extsTemps{".mon"};
  declareProperty(
      std::make_unique<Mantid::API::FileProperty>(
          "TemperatureFilename", "", Mantid::API::FileProperty::OptionalLoad,
          extsTemps),
      "The name of the temperature file to be loaded, this is optional as it "
      "will be automatically searched for if not provided.");

  declareProperty(
      std::make_unique<Mantid::API::WorkspaceProperty<Mantid::API::Workspace>>(
          "OutputWorkspace", "", Kernel::Direction::Output),
      "An output workspace.");

  declareProperty("SearchForTempFile", true,
                  "If no temp file has been given decide whether the algorithm "
                  "will search for the temperature file.");

  declareProperty("FirstGoodData", 0.0,
                  "First good data in the OutputWorkspace's spectra",
                  Kernel::Direction::Output);

  declareProperty("LastGoodData", 0.0,
                  "Last good data in the OutputWorkspace's spectra",
                  Kernel::Direction::Output);

  declareProperty("TimeZero", 0.0, "The TimeZero of the OutputWorkspace",
                  Kernel::Direction::Output);

  declareProperty("DataDeadTimeTable", 0,
                  "This property should not be set and is present to work with "
                  "the Muon GUI preprocessor.",
                  Kernel::Direction::Output);

  declareProperty("MainFieldDirection", 0,
                  "The field direction of the magnetic field on the instrument",
                  Kernel::Direction::Output);
}

void LoadPSIMuonBin::exec() {

  if (sizeof(float) != 4) {
    // assumptions made about the binary input won't work but since there is no
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
    outputWorkspace->getSpectrum(specNum).setDetectorID(specNum + 1);
  }

  assignOutputWorkspaceParticulars(outputWorkspace);

  // Set up for the Muon PreProcessor
  setProperty("OutputWorkspace", outputWorkspace);

  auto largestBinValue =
      outputWorkspace->x(0)[outputWorkspace->x(0).size() - 1];

  auto firstGoodDataSpecIndex = static_cast<int>(
      *std::max_element(m_header.firstGood, m_header.firstGood + 16));
  setProperty("FirstGoodData", outputWorkspace->x(0)[firstGoodDataSpecIndex]);

  // Since the arrray is all 0s before adding them this can't get min
  // element so just get first element
  auto lastGoodDataSpecIndex = static_cast<int>(m_header.lastGood[0]);
  setProperty("LastGoodData", outputWorkspace->x(0)[lastGoodDataSpecIndex]);

  double timeZero = 0.0;
  if (m_header.realT0[0] != 0) {
    timeZero = m_header.realT0[0];
  } else {
    timeZero = static_cast<double>(m_header.integerT0[0]);
  }

  // If timeZero is bigger than the largest bin assume it refers to a bin's
  // value
  if (timeZero > largestBinValue) {
    setProperty("TimeZero",
                outputWorkspace->x(0)[static_cast<int>(std::floor(timeZero))]);
  } else {
    setProperty("TimeZero", timeZero);
  }
}

std::string LoadPSIMuonBin::getFormattedDateTime(std::string date,
                                                 std::string time) {
  std::string year;
  if (date.size() == 11) {
    year = date.substr(7, 4);
  } else {
    year = "20" + date.substr(7, 2);
  }
  return year + "-" + months.find(date.substr(3, 3))->second + "-" +
         date.substr(0, 2) + "T" + time;
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
    // If no histogram bin width found calculate it
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
  m_histograms.reserve(m_header.numberOfHistograms);
  for (auto histogramIndex = 0; histogramIndex < m_header.numberOfHistograms;
       ++histogramIndex) {
    std::vector<double> nextHistogram;
    nextHistogram.reserve(m_header.lengthOfHistograms);
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
      nextHistogram.emplace_back(nextReadValue);
    }
    m_histograms.emplace_back(nextHistogram);
  }
}

void LoadPSIMuonBin::generateUnknownAxis() {
  // Create a x axis, assumption that m_histograms will all be the same size,
  // and that x will be 1 more in size than y
  for (auto xIndex = 0u; xIndex <= m_histograms[0].size(); ++xIndex) {
    m_xAxis.push_back(static_cast<double>(xIndex) * m_header.histogramBinWidth);
  }

  // Create Errors
  for (const auto &histogram : m_histograms) {
    std::vector<double> newEAxis;
    for (auto eIndex = 0u; eIndex < m_histograms[0].size(); ++eIndex) {
      newEAxis.push_back(sqrt(histogram[eIndex]));
    }
    m_eAxis.push_back(newEAxis);
  }
}

Mantid::API::Algorithm_sptr
LoadPSIMuonBin::createSampleLogAlgorithm(DataObjects::Workspace2D_sptr &ws) {
  Mantid::API::Algorithm_sptr logAlg = createChildAlgorithm("AddSampleLog");
  logAlg->setProperty("Workspace", ws);
  return logAlg;
}

void LoadPSIMuonBin::addToSampleLog(const std::string &logName,
                                    const std::string &logText,
                                    DataObjects::Workspace2D_sptr &ws) {
  auto alg = createSampleLogAlgorithm(ws);
  alg->setProperty("LogType", "String");
  alg->setProperty("LogName", logName);
  alg->setProperty("LogText", logText);
  alg->executeAsChildAlg();
}

void LoadPSIMuonBin::addToSampleLog(const std::string &logName,
                                    const double &logNumber,
                                    DataObjects::Workspace2D_sptr &ws) {
  auto alg = createSampleLogAlgorithm(ws);
  alg->setProperty("LogType", "Number");
  alg->setProperty("NumberType", "Double");
  alg->setProperty("LogName", logName);
  alg->setProperty("LogText", std::to_string(logNumber));
  alg->executeAsChildAlg();
}

void LoadPSIMuonBin::addToSampleLog(const std::string &logName,
                                    const int &logNumber,
                                    DataObjects::Workspace2D_sptr &ws) {
  auto alg = createSampleLogAlgorithm(ws);
  alg->setProperty("LogType", "Number");
  alg->setProperty("NumberType", "Int");
  alg->setProperty("LogName", logName);
  alg->setProperty("LogText", std::to_string(logNumber));
  alg->executeAsChildAlg();
}

void LoadPSIMuonBin::assignOutputWorkspaceParticulars(
    DataObjects::Workspace2D_sptr &outputWorkspace) {
  // Sort some workspace particulars
  outputWorkspace->setTitle(m_header.sample +
                            " - Run:" + std::to_string(m_header.numberOfRuns));

  // Set Run Property goodfrm
  outputWorkspace->mutableRun().addProperty(
      "goodfrm", static_cast<int>(m_header.lengthOfHistograms));
  outputWorkspace->mutableRun().addProperty(
      "run_number", static_cast<int>(m_header.numberOfRuns));

  // Set axis variables
  outputWorkspace->setYUnit("Counts");
  boost::shared_ptr<Kernel::Units::Label> lblUnit =
      boost::dynamic_pointer_cast<Kernel::Units::Label>(
          Kernel::UnitFactory::Instance().create("Label"));
  lblUnit->setLabel("Time", Kernel::Units::Symbol::Microsecond);
  outputWorkspace->getAxis(0)->unit() = lblUnit;

  // Set Start date and time and end date and time
  auto startDate = getFormattedDateTime(m_header.dateStart, m_header.timeStart);
  auto endDate = getFormattedDateTime(m_header.dateEnd, m_header.timeEnd);
  Mantid::Types::Core::DateAndTime start(startDate);
  Mantid::Types::Core::DateAndTime end(endDate);
  outputWorkspace->mutableRun().setStartAndEndTime(start, end);

  addToSampleLog("run_end", startDate, outputWorkspace);
  addToSampleLog("run_start", endDate, outputWorkspace);

  // Assume unit is at the end of the temperature
  boost::trim_right(m_header.temp);
  auto tempUnit = std::string(1, m_header.temp.at(m_header.temp.size() - 1));
  addToSampleLog("sample_temp_unit", tempUnit, outputWorkspace);

  // When poping the back off the temperature it is meant to remove the unit
  m_header.temp.pop_back();
  try {
    double temperature = std::stod(m_header.temp);
    addToSampleLog("sample_temp", temperature, outputWorkspace);
  } catch (std::invalid_argument &) {
    g_log.warning("The \"sample_temp\" could not be converted to a number for "
                  "the sample log so has been added as a string");
    addToSampleLog("sample_temp", m_header.temp, outputWorkspace);
  }

  // Add The other temperatures as log
  constexpr auto sizeOfTemps =
      sizeof(m_header.temperatures) / sizeof(*m_header.temperatures);
  for (auto tempNum = 1u; tempNum < sizeOfTemps + 1; ++tempNum) {
    if (m_header.temperatures[tempNum - 1] == 0)
      // Break out of for loop
      break;
    addToSampleLog("Spectra " + std::to_string(tempNum) + " Temperature",
                   m_header.temperatures[tempNum - 1], outputWorkspace);
    addToSampleLog("Spectra " + std::to_string(tempNum) +
                       " Temperature Deviation",
                   m_header.temperatureDeviation[tempNum - 1], outputWorkspace);
  }

  outputWorkspace->setComment(m_header.comment);
  addToSampleLog("Comment", m_header.comment, outputWorkspace);
  addToSampleLog("Length of run",
                 static_cast<double>(m_histograms[0].size()) *
                     m_header.histogramBinWidth,
                 outputWorkspace);

  boost::trim_right(m_header.field);
  auto fieldUnit = std::string(1, m_header.field.at(m_header.field.size() - 1));
  addToSampleLog("Field Unit", fieldUnit, outputWorkspace);
  m_header.field.pop_back();
  try {
    auto field = std::stod(m_header.field);
    addToSampleLog("sample_magn_field", field, outputWorkspace);
  } catch (std::invalid_argument &) {
    g_log.warning("The \"Field\" could not be converted to a number for "
                  "the sample log so has been added as a string");
    addToSampleLog("sample_magn_field", m_header.field, outputWorkspace);
  }

  // get scalar labels and set spectra accordingly
  constexpr auto sizeOfScalars =
      sizeof(m_header.scalars) / sizeof(*m_header.scalars);
  for (auto i = 0u; i < sizeOfScalars; ++i) {
    if (m_header.labels_scalars[i] == "NONE")
      // Break out of for loop
      break;
    addToSampleLog("Label Spectra " + std::to_string(i),
                   m_header.labels_scalars[i], outputWorkspace);
    addToSampleLog("Scalar Spectra " + std::to_string(i), m_header.scalars[i],
                   outputWorkspace);
  }

  addToSampleLog("Orientation", m_header.orientation, outputWorkspace);

  // first good and last good
  constexpr auto sizeOfFirstGood =
      sizeof(m_header.firstGood) / sizeof(*m_header.firstGood);
  for (size_t i = 0; i < sizeOfFirstGood; ++i) {
    if (m_header.firstGood[i] == 0)
      // Break out of for loop
      break;
    addToSampleLog("First good spectra " + std::to_string(i),
                   m_header.firstGood[i], outputWorkspace);
    addToSampleLog("Last good spectra " + std::to_string(i),
                   m_header.lastGood[i], outputWorkspace);
  }

  addToSampleLog("TDC Resolution", m_header.tdcResolution, outputWorkspace);
  addToSampleLog("TDC Overflow", m_header.tdcOverflow, outputWorkspace);
  addToSampleLog("Spectra Length", m_header.lengthOfHistograms,
                 outputWorkspace);
  addToSampleLog("Number of Spectra", m_header.numberOfHistograms,
                 outputWorkspace);
  addToSampleLog("Mon number of events", m_header.monNumberOfevents,
                 outputWorkspace);
  addToSampleLog("Mon Period", m_header.periodOfMon, outputWorkspace);

  if (m_header.monLow[0] == 0 && m_header.monHigh[0] == 0) {
    addToSampleLog("Mon Low", 0.0, outputWorkspace);
    addToSampleLog("Mon High", 0.0, outputWorkspace);
  } else {
    constexpr auto sizeOfMonLow =
        sizeof(m_header.monLow) / sizeof(*m_header.monLow);
    for (auto i = 0u; i < sizeOfMonLow; ++i) {
      if (m_header.monLow[i] == 0 || m_header.monHigh[i] == 0)
        // Break out of for loop
        break;
      addToSampleLog("Mon Low " + std::to_string(i), m_header.monLow[i],
                     outputWorkspace);
      addToSampleLog("Mon High" + std::to_string(i), m_header.monHigh[i],
                     outputWorkspace);
    }
  }

  addToSampleLog("Mon Deviation", m_header.monDeviation, outputWorkspace);

  if (m_header.realT0[0] != 0) {
    // 16 is the max size of realT0
    for (auto i = 0u; i < 16; ++i) {
      if (m_header.realT0[i] == 0)
        break;
      addToSampleLog("realT0 " + std::to_string(i), m_header.realT0[i],
                     outputWorkspace);
    }
  }

  // Read in the temperature file if provided/found
  try {
    readInTemperatureFile(outputWorkspace);
  } catch (std::invalid_argument &e) {
    g_log.warning("Temperature file was not be loaded: " +
                  std::string(e.what()));
  } catch (std::runtime_error &e) {
    g_log.warning("Temperature file was not be loaded:" +
                  std::string(e.what()));
  }
}

namespace {
std::string findTitlesFromLine(const std::string &line) {
  bool titlesFound = false;
  std::string foundTitles = "";
  for (const auto &charecter : line) {
    if (charecter == ':') {
      titlesFound = true;
    } else if (titlesFound) {
      foundTitles += charecter;
    }
  }
  return foundTitles;
}
} // namespace

void LoadPSIMuonBin::processTitleHeaderLine(const std::string &line) {
  auto foundTitles = findTitlesFromLine(line);
  boost::trim(foundTitles);
  if (foundTitles.find("\\") == std::string::npos) {
    boost::split(m_tempHeader.titles, foundTitles, boost::is_any_of(" "));
    m_tempHeader.delimeterOfTitlesIsBackSlash = false;
  } else {
    boost::split(m_tempHeader.titles, foundTitles, boost::is_any_of("\\"));
    m_tempHeader.delimeterOfTitlesIsBackSlash = true;
  }
}

void LoadPSIMuonBin::processDateHeaderLine(const std::string &line) {
  // Assume the date is added in the same place as always
  // line example = "! 2018-01-01 10:10:10"
  // date = 2018-01-01
  // _time = 10:10:10
  auto date = line.substr(2, 11);
  auto _time = line.substr(14, 8);
  m_tempHeader.startDateTime = getFormattedDateTime(date, _time);
}

void LoadPSIMuonBin::processHeaderLine(const std::string &line) {
  if (line.find("Title") != std::string::npos) {
    // Find sample log titles from the header
    processTitleHeaderLine(line);
  } else if (std::find(std::begin(psiMonths), std::end(psiMonths),
                       line.substr(5, 3)) != std::end(psiMonths)) {
    // If the line contains a Month in the PSI format then assume it conains a
    // date on the line. 5 is the index of the line that is where the month is
    // found and 3 is the length of the month.
    processDateHeaderLine(line);
  }
}

void LoadPSIMuonBin::readInTemperatureFileHeader(const std::string &contents) {
  const int uselessLines = 6;
  int lineNo = 0;
  std::string line = "";
  for (const auto charecter : contents) {
    if (charecter == '\n') {
      if (line[0] == '!' && lineNo > uselessLines) {
        processHeaderLine(line);
      } else if (line[0] != '!') {
        return;
      }
      ++lineNo;
      line = "";
    } else {
      line += charecter;
    }
  }
}

void LoadPSIMuonBin::processLine(const std::string &line,
                                 DataObjects::Workspace2D_sptr &ws) {
  std::vector<std::string> segments;
  boost::split(segments, line, boost::is_any_of("\\"));

  // 5 is the size that we expect vectors to be at this stage
  if (segments.size() != 5) {
    throw std::runtime_error(
        "Line does not have 5 segments delimited by \\: '" + line + "'");
  }
  const auto recordTime = segments[0];
  const auto numValues = std::stoi(segments[1]);
  std::vector<std::string> firstValues;
  boost::split(firstValues, segments[2], boost::is_any_of(" "));
  std::vector<std::string> secondValues;
  boost::split(secondValues, segments[3], boost::is_any_of(" "));

  // example recordTime = 10:10:10
  // 10 hours, 10 minutes, and 10 seconds. Hence the substr choices here.
  double secondsInRecordTime =
      (std::stoi(recordTime.substr(0, 2)) * 60 * 60) + // Hours
      (std::stoi(recordTime.substr(3, 2)) * 60) +      // Minutes
      (std::stoi(recordTime.substr(6, 2)));            // Seconds
  const auto timeLog = (Types::Core::DateAndTime(m_tempHeader.startDateTime) +
                        secondsInRecordTime)
                           .toISO8601String();

  Mantid::API::Algorithm_sptr logAlg = createChildAlgorithm("AddTimeSeriesLog");
  logAlg->setProperty("Workspace", ws);
  logAlg->setProperty("Time", timeLog);
  if (!m_tempHeader.delimeterOfTitlesIsBackSlash) {
    for (auto i = 0; i < numValues; ++i) {
      logAlg->setProperty("Name", "Temp_" + m_tempHeader.titles[i]);
      logAlg->setProperty("Type", "double");
      logAlg->setProperty("Value", firstValues[i]);
      logAlg->executeAsChildAlg();
    }
  } else {
    logAlg->setProperty("Name", "Temp_" + m_tempHeader.titles[0]);
    logAlg->setProperty("Type", "double");
    logAlg->setProperty("Value", firstValues[0]);
    logAlg->executeAsChildAlg();

    logAlg->setProperty("Name", "Temp_" + m_tempHeader.titles[1]);
    logAlg->setProperty("Type", "double");
    logAlg->setProperty("Value", secondValues[0]);
    logAlg->executeAsChildAlg();
  }
}

std::string LoadPSIMuonBin::detectTempFile() {
  const std::string binFileName = getPropertyValue("Filename");
  const Poco::Path fileDir(Poco::Path(binFileName).parent());

  Poco::SiblingsFirstRecursiveDirectoryIterator end;
  // 3 represents the maximum recursion depth for this search
  const uint16_t maxRecursionDepth = 3;
  for (Poco::SiblingsFirstRecursiveDirectoryIterator it(fileDir,
                                                        maxRecursionDepth);
       it != end; ++it) {
    // If it is not a directory, exists, has the extension '.mon', and it
    // contains the current run number.
    const Poco::Path path = it->path();
    if (!it->isDirectory() && it->exists() && path.getExtension() == "mon" &&
        path.getFileName().find(std::to_string(m_header.numberOfRuns)) !=
            std::string::npos) {
      return path.toString();
    }
  }
  return "";
}

void LoadPSIMuonBin::readInTemperatureFile(DataObjects::Workspace2D_sptr &ws) {
  std::string fileName = getPropertyValue("TemperatureFilename");
  const bool searchForTempFile = getProperty("SearchForTempFile");
  if (fileName == "" && searchForTempFile) {
    fileName = detectTempFile();
  }

  if (fileName == "") {
    throw std::invalid_argument(
        "No temperature file could be found/was provided");
  }

  g_log.notice("Temperature file in use by LoadPSIMuonBin: '" + fileName + "'");

  std::ifstream in(fileName, std::ios::in);
  std::string contents;
  in.seekg(0, std::ios::end);
  contents.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&contents[0], contents.size());
  in.close();

  readInTemperatureFileHeader(contents);

  std::string line = "";
  for (const auto &charecter : contents) {
    if (charecter == '\n') {
      if (line[0] == '!') {
        line = "";
      } else {
        processLine(line, ws);
        line = "";
      }
    } else {
      line += charecter;
    }
  }
}

} // namespace DataHandling
} // namespace Mantid
