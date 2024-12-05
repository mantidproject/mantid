
// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadPSIMuonBin.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadMuonStrategy.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BinaryStreamReader.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/UnitLabelTypes.h"

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>

namespace Mantid::DataHandling {

namespace {
const std::map<std::string, std::string> MONTHS{{"JAN", "01"}, {"FEB", "02"}, {"MAR", "03"}, {"APR", "04"},
                                                {"MAY", "05"}, {"JUN", "06"}, {"JUL", "07"}, {"AUG", "08"},
                                                {"SEP", "09"}, {"OCT", "10"}, {"NOV", "11"}, {"DEC", "12"}};
const std::vector<std::string> PSI_MONTHS{"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                          "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
constexpr int TEMPERATURE_FILE_MAX_SEARCH_DEPTH = 3;
constexpr auto TEMPERATURE_FILE_EXT = ".mon";
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
const std::string LoadPSIMuonBin::category() const { return "DataHandling\\PSI"; }

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

void LoadPSIMuonBin::init() {
  const std::vector<std::string> exts{".bin"};
  declareProperty(std::make_unique<Mantid::API::FileProperty>("Filename", "", Mantid::API::FileProperty::Load, exts),
                  "The name of the Bin file to load");

  const std::vector<std::string> extsTemps{".mon"};
  declareProperty(std::make_unique<Mantid::API::FileProperty>("TemperatureFilename", "",
                                                              Mantid::API::FileProperty::OptionalLoad, extsTemps),
                  "The name of the temperature file to be loaded, this is optional as it "
                  "will be automatically searched for if not provided.");

  declareProperty(std::make_unique<Mantid::API::WorkspaceProperty<Mantid::API::Workspace>>("OutputWorkspace", "",
                                                                                           Kernel::Direction::Output),
                  "An output workspace.");

  declareProperty("SearchForTempFile", true,
                  "If no temp file has been given decide whether the algorithm "
                  "will search for the temperature file.");

  declareProperty("FirstGoodData", 0.0, "First good data in the OutputWorkspace's spectra", Kernel::Direction::Output);

  declareProperty("LastGoodData", 0.0, "Last good data in the OutputWorkspace's spectra", Kernel::Direction::Output);

  declareProperty("TimeZero", 0.0, "The TimeZero of the OutputWorkspace", Kernel::Direction::Output);

  declareProperty(std::make_unique<Mantid::API::WorkspaceProperty<Mantid::API::Workspace>>(
                      "DeadTimeTable", "", Mantid::Kernel::Direction::Output, Mantid::API::PropertyMode::Optional),
                  "This property should only be set in the GUI and is present to work with "
                  "the Muon GUI preprocessor.");

  declareProperty("MainFieldDirection", 0, "The field direction of the magnetic field on the instrument",
                  Kernel::Direction::Output);

  declareProperty(std::make_unique<Kernel::ArrayProperty<double>>("TimeZeroList", Kernel::Direction::Output),
                  "A vector of time zero values");

  declareProperty(std::make_unique<Mantid::API::WorkspaceProperty<Mantid::API::Workspace>>(
                      "TimeZeroTable", "", Mantid::Kernel::Direction::Output, Mantid::API::PropertyMode::Optional),
                  "TableWorkspace of time zeros for each spectra");

  declareProperty("CorrectTime", true, "Boolean flag controlling whether time should be corrected by timezero.",
                  Kernel::Direction::Input);
  declareProperty(
      std::make_unique<Mantid::API::WorkspaceProperty<Mantid::API::Workspace>>(
          "DetectorGroupingTable", "", Mantid::Kernel::Direction::Output, Mantid::API::PropertyMode::Optional),
      "Table or a group of tables with information about the "
      "detector grouping stored in the file (if any).");
  auto mustBePositive = std::make_shared<Kernel::BoundedValidator<specnum_t>>();
  mustBePositive->setLower(0);
  declareProperty("SpectrumMin", static_cast<specnum_t>(0), mustBePositive,
                  "Index number of the first spectrum to read\n");
  declareProperty("SpectrumMax", static_cast<specnum_t>(EMPTY_INT()), mustBePositive,
                  "Index of last spectrum to read\n"
                  "(default the last spectrum)");
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
    throw std::runtime_error("Loaded file is not of PSIMuonBin type (First 2 bytes != 1N)");
  }

  readInHeader(streamReader);
  readInHistograms(streamReader);

  binFile.close();

  // Create the workspace stuff
  generateUnknownAxis();

  auto sizeOfXForHistograms = m_histograms[0].size() + 1;
  DataObjects::Workspace2D_sptr outputWorkspace = DataObjects::create<DataObjects::Workspace2D>(
      m_header.numberOfHistograms,
      Mantid::HistogramData::Histogram(Mantid::HistogramData::BinEdges(sizeOfXForHistograms)));

  for (auto specNum = 0u; specNum < m_histograms.size(); ++specNum) {
    outputWorkspace->mutableX(specNum) = m_xAxis;
    outputWorkspace->mutableY(specNum) = m_histograms[specNum];
    outputWorkspace->mutableE(specNum) = m_eAxis[specNum];
    outputWorkspace->getSpectrum(specNum).setDetectorID(specNum + 1);
  }

  assignOutputWorkspaceParticulars(outputWorkspace);

  // Set up for the Muon PreProcessor
  // create empty dead time table
  makeDeadTimeTable(m_histograms.size());

  auto largestBinValue = outputWorkspace->x(0)[outputWorkspace->x(0).size() - 1];

  // Since the arrray is all 0s before adding them this can't get min
  // element so just get first element
  auto lastGoodDataSpecIndex = static_cast<int>(m_header.lastGood[0]);
  setProperty("LastGoodData", outputWorkspace->x(0)[lastGoodDataSpecIndex]);

  double timeZero = 0.0;
  std::vector<double> timeZeroList;
  if (m_header.realT0[0] != 0) {
    timeZero = *std::max_element(std::begin(m_header.realT0), std::end(m_header.realT0));
    timeZeroList = std::vector<double>(std::begin(m_header.realT0), std::begin(m_header.realT0) + m_histograms.size());
  } else {
    timeZero = static_cast<double>(*std::max_element(std::begin(m_header.integerT0), std::end(m_header.integerT0)));
    timeZeroList =
        std::vector<double>(std::begin(m_header.integerT0), std::begin(m_header.integerT0) + m_histograms.size());
  }

  // If timeZero is bigger than the largest bin assume it refers to a bin's
  // value
  double absTimeZero = timeZero;
  std::vector<double> correctedTimeZeroList;
  if (timeZero > largestBinValue) {
    absTimeZero = outputWorkspace->x(0)[static_cast<int>(std::floor(timeZero))];
    std::transform(timeZeroList.cbegin(), timeZeroList.cend(), std::back_inserter(correctedTimeZeroList),
                   [&outputWorkspace](const auto timeZeroValue) {
                     return outputWorkspace->x(0)[static_cast<int>(std::floor(timeZeroValue))];
                   });
  } else {
    correctedTimeZeroList = timeZeroList;
  }

  setProperty("TimeZero", absTimeZero);
  setProperty("TimeZeroList", correctedTimeZeroList);

  // create time zero table
  if (!getPropertyValue("TimeZeroTable").empty()) {
    auto table = createTimeZeroTable(m_histograms.size(), correctedTimeZeroList);
    setProperty("TimeZeroTable", table);
  }

  auto firstGoodDataSpecIndex = static_cast<int>(*std::max_element(m_header.firstGood, m_header.firstGood + 16));

  setProperty("FirstGoodData", outputWorkspace->x(0)[firstGoodDataSpecIndex]);

  // Time zero is when the pulse starts.
  // The pulse should be at t=0 to be like ISIS data
  // manually offset the data
  auto correctTime = getProperty("CorrectTime");
  if (correctTime) {
    for (auto specNum = 0u; specNum < m_histograms.size(); ++specNum) {
      auto &xData = outputWorkspace->mutableX(specNum);
      std::transform(xData.cbegin(), xData.cend(), xData.begin(),
                     [absTimeZero](const auto &xValue) { return xValue - absTimeZero; });
    }
  }
  setProperty("OutputWorkspace", extractSpectra(outputWorkspace));

  // Set DetectorGroupingTable if needed
  setDetectorGroupingTable(m_histograms.size());
}

void LoadPSIMuonBin::setDetectorGroupingTable(const size_t &numSpec) {
  if (getPropertyValue("DetectorGroupingTable").empty())
    return;
  Mantid::DataObjects::TableWorkspace_sptr detectorTable =
      std::dynamic_pointer_cast<Mantid::DataObjects::TableWorkspace>(
          Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace"));
  detectorTable->addColumn("vector_int", "detector");
  for (size_t i = 0; i < numSpec; i++) {
    std::vector<int> dets{static_cast<int>(i) + 1};
    Mantid::API::TableRow row = detectorTable->appendRow();
    row << dets;
  }
  setProperty("DetectorGroupingTable", detectorTable);
}

void LoadPSIMuonBin::makeDeadTimeTable(const size_t &numSpec) {
  if (getPropertyValue("DeadTimeTable").empty())
    return;
  Mantid::DataObjects::TableWorkspace_sptr deadTimeTable =
      std::dynamic_pointer_cast<Mantid::DataObjects::TableWorkspace>(
          Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace"));
  assert(deadTimeTable);
  deadTimeTable->addColumn("int", "spectrum");
  deadTimeTable->addColumn("double", "dead-time");

  for (size_t i = 0; i < numSpec; i++) {
    Mantid::API::TableRow row = deadTimeTable->appendRow();
    row << static_cast<int>(i) + 1 << 0.0;
  }
  setProperty("DeadTimeTable", deadTimeTable);
}

std::string LoadPSIMuonBin::getFormattedDateTime(const std::string &date, const std::string &time) {
  std::string year;
  if (date.size() == 11) {
    year = date.substr(7, 4);
  } else {
    year = "20" + date.substr(7, 2);
  }
  return year + "-" + MONTHS.find(date.substr(3, 3))->second + "-" + date.substr(0, 2) + "T" + time;
}

void LoadPSIMuonBin::readSingleVariables(Mantid::Kernel::BinaryStreamReader &streamReader) {
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
        static_cast<float>((625.E-6) / 8. * pow(static_cast<float>(2.), static_cast<float>(m_header.tdcResolution)));
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

void LoadPSIMuonBin::readStringVariables(Mantid::Kernel::BinaryStreamReader &streamReader) {
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

void LoadPSIMuonBin::readArrayVariables(Mantid::Kernel::BinaryStreamReader &streamReader) {
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
    streamReader.read(m_header.labelsOfHistograms[i], 4);

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

void LoadPSIMuonBin::readInHeader(Mantid::Kernel::BinaryStreamReader &streamReader) {
  readSingleVariables(streamReader);
  readStringVariables(streamReader);
  readArrayVariables(streamReader);
}

void LoadPSIMuonBin::readInHistograms(Mantid::Kernel::BinaryStreamReader &streamReader) {
  constexpr auto sizeInt32_t = sizeof(int32_t);
  const auto headerSize = 1024;
  m_histograms.resize(m_header.numberOfHistograms);
  for (auto histogramIndex = 0; histogramIndex < m_header.numberOfHistograms; ++histogramIndex) {
    const auto offset = histogramIndex * m_header.numberOfDataRecordsHistogram * m_header.lengthOfDataRecordsBin;
    std::vector<double> &nextHistogram = m_histograms[histogramIndex];
    streamReader.moveStreamToPosition(offset * sizeInt32_t + headerSize);
    nextHistogram.reserve(m_header.lengthOfHistograms);
    for (auto rowIndex = 0; rowIndex < m_header.lengthOfHistograms; ++rowIndex) {
      int32_t nextReadValue;
      streamReader >> nextReadValue;
      nextHistogram.emplace_back(nextReadValue);
    }
  }
}

void LoadPSIMuonBin::generateUnknownAxis() {
  // Create a x axis, assumption that m_histograms will all be the same size,
  // and that x will be 1 more in size than y
  for (auto xIndex = 0u; xIndex <= m_histograms[0].size(); ++xIndex) {
    m_xAxis.emplace_back(static_cast<double>(xIndex) * m_header.histogramBinWidth);
  }

  // Create Errors
  for (const auto &histogram : m_histograms) {
    std::vector<double> newEAxis;
    for (auto eIndex = 0u; eIndex < m_histograms[0].size(); ++eIndex) {
      newEAxis.emplace_back(sqrt(histogram[eIndex]));
    }
    m_eAxis.emplace_back(newEAxis);
  }
}

Mantid::API::Algorithm_sptr LoadPSIMuonBin::createSampleLogAlgorithm(DataObjects::Workspace2D_sptr &ws) {
  Mantid::API::Algorithm_sptr logAlg = createChildAlgorithm("AddSampleLog");
  logAlg->setProperty("Workspace", ws);
  return logAlg;
}

API::MatrixWorkspace_sptr LoadPSIMuonBin::extractSpectra(DataObjects::Workspace2D_sptr &ws) {
  Mantid::API::Algorithm_sptr alg = createChildAlgorithm("ExtractSpectra");
  alg->setProperty("InputWorkspace", ws);
  alg->setProperty("OutputWorkspace", "not_used");
  alg->setProperty("StartWorkspaceIndex", getPropertyValue("SpectrumMin"));
  alg->setProperty("EndWorkspaceIndex", getPropertyValue("SpectrumMax"));
  alg->executeAsChildAlg();
  return alg->getProperty("OutputWorkspace");
}

void LoadPSIMuonBin::addToSampleLog(const std::string &logName, const std::string &logText,
                                    DataObjects::Workspace2D_sptr &ws) {
  auto alg = createSampleLogAlgorithm(ws);
  alg->setProperty("LogType", "String");
  alg->setProperty("LogName", logName);
  alg->setProperty("LogText", logText);
  alg->executeAsChildAlg();
}

void LoadPSIMuonBin::addToSampleLog(const std::string &logName, const double &logNumber,
                                    DataObjects::Workspace2D_sptr &ws) {
  auto alg = createSampleLogAlgorithm(ws);
  alg->setProperty("LogType", "Number");
  alg->setProperty("NumberType", "Double");
  alg->setProperty("LogName", logName);
  alg->setProperty("LogText", std::to_string(logNumber));
  alg->executeAsChildAlg();
}

void LoadPSIMuonBin::addToSampleLog(const std::string &logName, const int &logNumber,
                                    DataObjects::Workspace2D_sptr &ws) {
  auto alg = createSampleLogAlgorithm(ws);
  alg->setProperty("LogType", "Number");
  alg->setProperty("NumberType", "Int");
  alg->setProperty("LogName", logName);
  alg->setProperty("LogText", std::to_string(logNumber));
  alg->executeAsChildAlg();
}

void LoadPSIMuonBin::assignOutputWorkspaceParticulars(DataObjects::Workspace2D_sptr &outputWorkspace) {
  // Sort some workspace particulars
  outputWorkspace->setTitle(m_header.sample + " - Run:" + std::to_string(m_header.numberOfRuns));

  // Set Run Property goodfrm
  outputWorkspace->mutableRun().addProperty("goodfrm", static_cast<int>(m_header.lengthOfHistograms));
  outputWorkspace->mutableRun().addProperty("run_number", static_cast<int>(m_header.numberOfRuns));

  // Set axis variables
  outputWorkspace->setYUnit("Counts");
  std::shared_ptr<Kernel::Units::Label> lblUnit =
      std::dynamic_pointer_cast<Kernel::Units::Label>(Kernel::UnitFactory::Instance().create("Label"));
  lblUnit->setLabel("Time", Kernel::Units::Symbol::Microsecond);
  outputWorkspace->getAxis(0)->unit() = lblUnit;

  // Set Start date and time and end date and time
  auto startDate = getFormattedDateTime(m_header.dateStart, m_header.timeStart);
  auto endDate = getFormattedDateTime(m_header.dateEnd, m_header.timeEnd);
  try {
    Mantid::Types::Core::DateAndTime start(startDate);
    Mantid::Types::Core::DateAndTime end(endDate);
    outputWorkspace->mutableRun().setStartAndEndTime(start, end);
  } catch (const std::logic_error &) {
    Mantid::Types::Core::DateAndTime start;
    Mantid::Types::Core::DateAndTime end;
    outputWorkspace->mutableRun().setStartAndEndTime(start, end);
    g_log.warning("The date in the .bin file was invalid");
  }

  addToSampleLog("run_end", endDate, outputWorkspace);
  addToSampleLog("run_start", startDate, outputWorkspace);

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
  constexpr auto sizeOfTemps = sizeof(m_header.temperatures) / sizeof(*m_header.temperatures);
  for (auto tempNum = 1u; tempNum < sizeOfTemps + 1; ++tempNum) {
    if (m_header.temperatures[tempNum - 1] == 0)
      // Break out of for loop
      break;
    addToSampleLog("Spectra " + std::to_string(tempNum) + " Temperature", m_header.temperatures[tempNum - 1],
                   outputWorkspace);
    addToSampleLog("Spectra " + std::to_string(tempNum) + " Temperature Deviation",
                   m_header.temperatureDeviation[tempNum - 1], outputWorkspace);
  }

  outputWorkspace->setComment(m_header.comment);
  addToSampleLog("Comment", m_header.comment, outputWorkspace);
  addToSampleLog("Length of run", static_cast<double>(m_histograms[0].size()) * m_header.histogramBinWidth,
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
  constexpr auto sizeOfScalars = sizeof(m_header.scalars) / sizeof(*m_header.scalars);
  for (auto i = 0u; i < sizeOfScalars; ++i) {
    if (m_header.labels_scalars[i] == "NONE")
      // Break out of for loop
      break;
    addToSampleLog("Scalar Label Spectra " + std::to_string(i), m_header.labels_scalars[i], outputWorkspace);
    addToSampleLog("Scalar Spectra " + std::to_string(i), m_header.scalars[i], outputWorkspace);
  }

  constexpr auto sizeOfLabels = sizeof(m_header.labelsOfHistograms) / sizeof(*m_header.labelsOfHistograms);
  for (auto i = 0u; i < sizeOfLabels; ++i) {
    if (m_header.labelsOfHistograms[i] == "")
      break;
    std::string labelName = m_header.labelsOfHistograms[i];
    // if empty name is present (i.e. just empty space)
    // replace with default name:
    // group_specNum
    const bool isSpace = labelName.find_first_not_of(" ") == std::string::npos;
    std::string label = isSpace ? "group_" + std::to_string(i + 1) : m_header.labelsOfHistograms[i];

    addToSampleLog("Label Spectra " + std::to_string(i), label, outputWorkspace);
  }

  addToSampleLog("Orientation", m_header.orientation, outputWorkspace);

  // first good and last good
  constexpr auto sizeOfFirstGood = sizeof(m_header.firstGood) / sizeof(*m_header.firstGood);
  for (size_t i = 0; i < sizeOfFirstGood; ++i) {
    if (m_header.firstGood[i] == 0)
      // Break out of for loop
      break;
    addToSampleLog("First good spectra " + std::to_string(i), m_header.firstGood[i], outputWorkspace);
    addToSampleLog("Last good spectra " + std::to_string(i), m_header.lastGood[i], outputWorkspace);
  }

  addToSampleLog("TDC Resolution", m_header.tdcResolution, outputWorkspace);
  addToSampleLog("TDC Overflow", m_header.tdcOverflow, outputWorkspace);
  addToSampleLog("Spectra Length", m_header.lengthOfHistograms, outputWorkspace);
  addToSampleLog("Number of Spectra", m_header.numberOfHistograms, outputWorkspace);
  addToSampleLog("Mon number of events", m_header.monNumberOfevents, outputWorkspace);
  addToSampleLog("Mon Period", m_header.periodOfMon, outputWorkspace);

  if (m_header.monLow[0] == 0 && m_header.monHigh[0] == 0) {
    addToSampleLog("Mon Low", 0.0, outputWorkspace);
    addToSampleLog("Mon High", 0.0, outputWorkspace);
  } else {
    constexpr auto sizeOfMonLow = sizeof(m_header.monLow) / sizeof(*m_header.monLow);
    for (auto i = 0u; i < sizeOfMonLow; ++i) {
      if (m_header.monLow[i] == 0 || m_header.monHigh[i] == 0)
        // Break out of for loop
        break;
      addToSampleLog("Mon Low " + std::to_string(i), m_header.monLow[i], outputWorkspace);
      addToSampleLog("Mon High" + std::to_string(i), m_header.monHigh[i], outputWorkspace);
    }
  }

  addToSampleLog("Mon Deviation", m_header.monDeviation, outputWorkspace);

  if (m_header.realT0[0] != 0) {
    // 16 is the max size of realT0
    for (auto i = 0u; i < 16; ++i) {
      if (m_header.realT0[i] == 0)
        break;
      addToSampleLog("realT0 " + std::to_string(i), m_header.realT0[i], outputWorkspace);
    }
  }

  if (m_header.integerT0[0] != 0) {
    // 16 is the max size of integerT0
    for (auto i = 0u; i < 16; ++i) {
      if (m_header.integerT0[i] == 0)
        break;
      addToSampleLog("integerT0 " + std::to_string(i), m_header.integerT0[i], outputWorkspace);
    }
  }

  // Read in the temperature file if provided/found
  try {
    readInTemperatureFile(outputWorkspace);
  } catch (std::invalid_argument &e) {
    g_log.warning("Temperature file was not be loaded: " + std::string(e.what()));
  } catch (std::runtime_error &e) {
    g_log.warning("Temperature file was not be loaded:" + std::string(e.what()));
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
  } else if (std::find(std::begin(PSI_MONTHS), std::end(PSI_MONTHS), line.substr(5, 3)) != std::end(PSI_MONTHS)) {
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
      if (line.empty() || line[0] != '!') {
        return;
      } else if (lineNo > uselessLines) {
        processHeaderLine(line);
      }
      ++lineNo;
      line = "";
    } else {
      line += charecter;
    }
  }
}

void LoadPSIMuonBin::processLine(const std::string &line, DataObjects::Workspace2D_sptr &ws) {
  std::vector<std::string> segments;
  boost::split(segments, line, boost::is_any_of("\\"));

  // 5 is the size that we expect vectors to be at this stage
  if (segments.size() != 5) {
    throw std::runtime_error("Line does not have 5 segments delimited by \\: '" + line + "'");
  }
  const auto recordTime = segments[0];
  const auto numValues = std::stoi(segments[1]);
  std::vector<std::string> firstValues;
  boost::split(firstValues, segments[2], boost::is_any_of(" "));
  std::vector<std::string> secondValues;
  boost::split(secondValues, segments[3], boost::is_any_of(" "));

  // example recordTime = 10:10:10
  // 10 hours, 10 minutes, and 10 seconds. Hence the substr choices here.
  double secondsInRecordTime = (std::stoi(recordTime.substr(0, 2)) * 60 * 60) + // Hours
                               (std::stoi(recordTime.substr(3, 2)) * 60) +      // Minutes
                               (std::stoi(recordTime.substr(6, 2)));            // Seconds
  const auto timeLog = (Types::Core::DateAndTime(m_tempHeader.startDateTime) + secondsInRecordTime).toISO8601String();

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
  // Perform a breadth-first search starting from
  // directory containing the main file. The search has
  // a fixed limited depth to ensure we don't accidentally
  // crawl the while filesystem.
  namespace fs = std::filesystem;
  const fs::path searchDir{fs::path{getPropertyValue("Filename")}.parent_path()};

  std::deque<fs::path> queue{fs::path{searchDir}};
  while (!queue.empty()) {
    const auto first = queue.front();
    queue.pop_front();
    for (fs::directory_iterator dirIter{first}; dirIter != fs::directory_iterator(); ++dirIter) {
      const auto &entry{dirIter->path()};

      if (fs::is_directory(entry)) {
        const auto relPath{entry.lexically_relative(searchDir)};
        if (std::distance(relPath.begin(), relPath.end()) < TEMPERATURE_FILE_MAX_SEARCH_DEPTH) {
          // save the directory for searching when we have exhausted
          // the file entries at this level
          queue.push_back(entry);
        }
      } else if (entry.extension() == TEMPERATURE_FILE_EXT &&
                 entry.filename().string().find(std::to_string(m_header.numberOfRuns)) != std::string::npos) {
        return entry.string();
      }
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
    throw std::invalid_argument("No temperature file could be found/was provided");
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
  for (const auto &character : contents) {
    if (character == '\n') {
      if (!line.empty() && line[0] == '!') {
        line = "";
      } else {
        processLine(line, ws);
        line = "";
      }
    } else {
      line += character;
    }
  }
}

} // namespace Mantid::DataHandling
