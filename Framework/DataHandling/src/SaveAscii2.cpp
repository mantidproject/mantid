// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveAscii2.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/UnitConversion.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/VisibleWhenProperty.h"

#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <fstream>
#include <set>

namespace Mantid::DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveAscii2)

using namespace Kernel;
using namespace API;

/// Empty constructor
SaveAscii2::SaveAscii2()
    : m_separatorIndex(), m_nBins(0), m_sep(), m_writeDX(false), m_writeID(false), m_isCommonBins(false),
      m_writeSpectrumAxisValue(false), m_ws() {}

/// Initialisation method.
void SaveAscii2::init() {
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("InputWorkspace", "", Direction::Input),
                  "The name of the workspace containing the data you want to save to a "
                  "Ascii file.");

  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Save, m_asciiExts),
                  "The filename of the output Ascii file.");

  auto mustBePositive = std::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(1);
  auto mustBeZeroGreater = std::make_shared<BoundedValidator<int>>();
  mustBeZeroGreater->setLower(0);
  declareProperty("WorkspaceIndexMin", EMPTY_INT(), mustBeZeroGreater,
                  "The starting workspace index. Ignored for Table Workspaces.");
  declareProperty("WorkspaceIndexMax", EMPTY_INT(), mustBeZeroGreater,
                  "The ending workspace index. Ignored for Table Workspaces.");
  declareProperty(std::make_unique<ArrayProperty<int>>("SpectrumList"),
                  "List of workspace indices to save. Ignored for Table Workspaces.");
  declareProperty("Precision", EMPTY_INT(), mustBePositive, "Precision of output double values.");
  declareProperty("ScientificFormat", false,
                  "If true, the values will be "
                  "written to the file in "
                  "scientific notation.");
  declareProperty("WriteXError", false,
                  "If true, the error on X will be written as the fourth "
                  "column. Ignored for Table Workspaces.");
  declareProperty("WriteSpectrumID", true,
                  "If false, the spectrum No will not be written for "
                  "single-spectrum workspaces. "
                  "It is always written for workspaces with multiple spectra, "
                  "unless spectrum axis value is written. Ignored for Table Workspaces.");

  declareProperty("CommentIndicator", "#", "Character(s) to put in front of comment lines.");

  // For the ListValidator
  std::string spacers[6][2] = {{"CSV", ","},   {"Tab", "\t"},      {"Space", " "},
                               {"Colon", ":"}, {"SemiColon", ";"}, {"UserDefined", "UserDefined"}};
  std::vector<std::string> sepOptions;
  for (auto &spacer : spacers) {
    std::string option = spacer[0];
    m_separatorIndex.insert(std::pair<std::string, std::string>(option, spacer[1]));
    sepOptions.emplace_back(option);
  }

  declareProperty("Separator", "CSV", std::make_shared<StringListValidator>(sepOptions),
                  "The separator between data columns in the data file. The "
                  "possible values are \"CSV\", \"Tab\", "
                  "\"Space\", \"SemiColon\", \"Colon\" or \"UserDefined\".");

  declareProperty(std::make_unique<PropertyWithValue<std::string>>("CustomSeparator", "", Direction::Input),
                  "If present, will override any specified choice given to Separator.");

  setPropertySettings("CustomSeparator",
                      std::make_unique<VisibleWhenProperty>("Separator", IS_EQUAL_TO, "UserDefined"));
  getPointerToProperty("CustomSeparator")->setAutoTrim(false);
  declareProperty("ColumnHeader", true,
                  "If true, put column headers into file. Even if false, a header"
                  "is automatically added if the workspace is Distribution = true.");

  declareProperty("SpectrumMetaData", "",
                  "A comma separated list that defines data that describes "
                  "each spectrum in a workspace. The valid options for this "
                  "are: SpectrumNumber,Q,Angle. Ignored for Table Workspaces.");

  declareProperty("AppendToFile", false, "If true, don't overwrite the file. Append to the end of it. ");

  declareProperty("RaggedWorkspace", true,
                  "If true, ensure that more than one xspectra is used. "
                  "Ignored for Table Workspaces."); // in testing

  declareProperty("WriteSpectrumAxisValue", false,
                  "Write the spectrum axis value if requested. Ignored for "
                  "Table Workspaces.");

  declareProperty(std::make_unique<ArrayProperty<std::string>>("LogList"),
                  "List of logs to write to the file header. Ignored for Table "
                  "Workspaces.");

  declareProperty("OneSpectrumPerFile", false, "If true, each spectrum will be saved to an individual file");
}

/**
 *   Executes the algorithm.
 */
void SaveAscii2::exec() {
  // Get the workspace
  Workspace_const_sptr ws = getProperty("InputWorkspace");
  m_ws = std::dynamic_pointer_cast<const MatrixWorkspace>(ws);
  ITableWorkspace_const_sptr tws = std::dynamic_pointer_cast<const ITableWorkspace>(ws);
  IMDHistoWorkspace_const_sptr mdws = std::dynamic_pointer_cast<const IMDHistoWorkspace>(ws);

  // Get the properties valid for all workspaces
  const bool writeHeader = getProperty("ColumnHeader");
  const bool appendToFile = getProperty("AppendToFile");
  std::string filename = getProperty("Filename");
  int prec = getProperty("Precision");
  bool scientific = getProperty("ScientificFormat");
  std::string comment = getPropertyValue("CommentIndicator");

  const std::string choice = getPropertyValue("Separator");
  const std::string custom = getPropertyValue("CustomSeparator");
  // If the custom separator property is not empty, then we use that under
  // any circumstance.
  if (!custom.empty()) {
    m_sep = custom;
  }
  // Else if the separator drop down choice is not UserDefined then we use
  // that.
  else if (choice != "UserDefined") {
    auto it = m_separatorIndex.find(choice);
    m_sep = it->second;
  }
  // If we still have nothing, then we are forced to use a default.
  if (m_sep.empty()) {
    g_log.notice() << "\"UserDefined\" has been selected, but no custom "
                      "separator has been entered."
                      " Using default instead.";
    m_sep = ",";
  }

  // handle table and 1 histo cuts workspaces
  if (tws) {
    return writeTableWorkspace(tws, filename, appendToFile, writeHeader, prec, scientific, comment);
  } else if (mdws) {
    return write1DHistoCut(mdws, filename, appendToFile, writeHeader, prec, scientific, comment);
  }

  if (!m_ws) {
    throw std::runtime_error("SaveAscii does not know how to save this workspace type, " + ws->getName());
  }

  // Get the properties valid for matrix workspaces
  std::vector<int> spec_list = getProperty("SpectrumList");
  const int spec_min = getProperty("WorkspaceIndexMin");
  const int spec_max = getProperty("WorkspaceIndexMax");
  m_writeSpectrumAxisValue = getProperty("WriteSpectrumAxisValue");
  m_writeDX = getProperty("WriteXError");

  m_writeID = getProperty("WriteSpectrumID");
  std::string metaDataString = getPropertyValue("SpectrumMetaData");
  if (!metaDataString.empty()) {
    m_metaData = stringListToVector(metaDataString);
    auto containsSpectrumNumber = findElementInUnorderedStringVector(m_metaData, "spectrumnumber");
    if (containsSpectrumNumber) {
      try {
        m_ws->getSpectrumToWorkspaceIndexMap();
      } catch (const std::runtime_error &) {
        throw std::runtime_error("SpectrumNumber is present in "
                                 "SpectrumMetaData but the workspace does not "
                                 "have a SpectrumAxis.");
      }
    }
  }
  if (m_writeID) {
    auto containsSpectrumNumber = findElementInUnorderedStringVector(m_metaData, "spectrumnumber");
    if (!containsSpectrumNumber) {
      auto firstIter = m_metaData.begin();
      m_metaData.insert(firstIter, "spectrumnumber");
    }
  }

  if (m_writeSpectrumAxisValue) {
    auto spectrumAxis = m_ws->getAxis(1);
    if (dynamic_cast<BinEdgeAxis *>(spectrumAxis)) {
      m_axisProxy = std::make_unique<AxisHelper::BinEdgeAxisProxy>(spectrumAxis);
    } else {
      m_axisProxy = std::make_unique<AxisHelper::AxisProxy>(spectrumAxis);
    }
  }

  // e + and - are included as they're part of the scientific notation
  if (!boost::regex_match(m_sep.begin(), m_sep.end(), boost::regex("[^0-9e+-]+", boost::regex::perl))) {
    throw std::invalid_argument("Separators cannot contain numeric characters, "
                                "plus signs, hyphens or 'e'");
  }

  if (comment.at(0) == m_sep.at(0) ||
      !boost::regex_match(comment.begin(), comment.end(),
                          boost::regex("[^0-9e" + m_sep + "+-]+", boost::regex::perl))) {
    throw std::invalid_argument("Comment markers cannot contain numeric "
                                "characters, plus signs, hyphens,"
                                " 'e' or the selected separator character");
  }

  // Create an spectra index list for output
  std::set<int> idx;

  auto nSpectra = static_cast<int>(m_ws->getNumberHistograms());
  m_nBins = static_cast<int>(m_ws->blocksize());
  m_isCommonBins = m_ws->isCommonBins(); // checking for ragged workspace

  // Add spectra interval into the index list
  if (spec_max != EMPTY_INT() && spec_min != EMPTY_INT()) {
    if (spec_min >= nSpectra || spec_max >= nSpectra || spec_min < 0 || spec_max < 0 || spec_min > spec_max) {
      throw std::invalid_argument("Inconsistent spectra interval");
    }
    for (int i = spec_min; i <= spec_max; i++) {
      idx.insert(i);
    }
  }
  // figure out how to read in readX and have them be seperate lists

  // Add spectra list into the index list
  if (!spec_list.empty()) {
    for (auto &spec : spec_list) {
      if (spec >= nSpectra) {
        throw std::invalid_argument("Inconsistent spectra list");
      } else {
        idx.insert(spec);
      }
    }
  }

  // if no interval or spectra list, take all of them
  if (idx.empty()) {
    for (int i = 0; i < nSpectra; i++) {
      idx.insert(i);
    }
  }

  if (m_nBins == 0 || nSpectra == 0) {
    throw std::runtime_error("Trying to save an empty workspace");
  }

  const bool oneSpectrumPerFile = getProperty("OneSpectrumPerFile");

  Progress progress(this, 0.0, 1.0, idx.size());

  // populate the meta data map
  if (!m_metaData.empty()) {
    populateAllMetaData();
  }

  const bool isDistribution = m_ws->isDistribution();
  auto idxIt = idx.begin();
  while (idxIt != idx.end()) {
    std::string currentFilename;
    if (oneSpectrumPerFile)
      currentFilename = createSpectrumFilename(*idxIt);
    else
      currentFilename = filename;

    std::ofstream file(currentFilename, (appendToFile ? std::ios::app : std::ios::out));

    if (file.bad()) {
      throw Exception::FileError("Unable to create file: ", currentFilename);
    }
    // Set the number precision
    if (prec != EMPTY_INT()) {
      file.precision(prec);
    }
    if (scientific) {
      file << std::scientific;
    }
    const std::vector<std::string> logList = getProperty("LogList");
    if (!logList.empty()) {
      writeFileHeader(logList, file);
    }
    if (writeHeader || isDistribution) {
      file << comment << " X " << m_sep << " Y " << m_sep << " E";
      if (m_writeDX) {
        file << " " << m_sep << " DX";
      }
      file << " Distribution=" << (isDistribution ? "true" : "false");
      file << '\n';
    }

    // data writing
    if (oneSpectrumPerFile) {
      writeSpectrum(*idxIt, file);
      progress.report();
      idxIt++;
    } else {
      while (idxIt != idx.end()) {
        writeSpectrum(*idxIt, file);
        progress.report();
        idxIt++;
      }
    }

    file.unsetf(std::ios_base::floatfield);
    file.close();
  }
}

/** Create the filename used for the export of a specific spectrum. Valid only
 *  when spectra are exported in separate files.
 *
 *  @param workspaceIndex :: index of the corresponding spectrum
 */

std::string SaveAscii2::createSpectrumFilename(size_t workspaceIndex) {
  std::string filename = getProperty("Filename");
  size_t extPosition{std::string::npos};
  for (const std::string &ext : m_asciiExts) {
    extPosition = filename.find(ext);
    if (extPosition != std::string::npos)
      break;
  }
  if (extPosition == std::string::npos)
    extPosition = filename.size();

  std::ostringstream ss;
  ss << std::string(filename, 0, extPosition) << "_" << workspaceIndex;
  auto axis = m_ws->getAxis(1);
  if (axis->isNumeric()) {
    auto binEdgeAxis = dynamic_cast<BinEdgeAxis *>(axis);
    if (binEdgeAxis)
      ss << "_" << binEdgeAxis->label(workspaceIndex) << axis->unit()->label().ascii();
    else
      ss << "_" << axis->getValue(workspaceIndex) << axis->unit()->label().ascii();
  } else if (axis->isText())
    ss << "_" << axis->label(workspaceIndex);
  ss << std::string(filename, extPosition);

  return ss.str();
}

/** Writes a spectrum to the file using a workspace index
 *
 * @param wsIndex :: an integer relating to a workspace index
 * @param file :: the file writer object
 */
void SaveAscii2::writeSpectrum(const int &wsIndex, std::ofstream &file) {

  if (m_writeSpectrumAxisValue) {
    file << m_axisProxy->getCentre(wsIndex) << '\n';
  } else {
    for (auto iter = m_metaData.begin(); iter != m_metaData.end(); ++iter) {
      auto value = m_metaDataMap[*iter][wsIndex];
      file << value;
      if (iter != m_metaData.end() - 1) {
        file << " " << m_sep << " ";
      }
    }
    file << '\n';
  }
  auto pointDeltas = m_ws->pointStandardDeviations(0);
  auto points0 = m_ws->points(0);
  auto pointsSpec = m_ws->points(wsIndex);
  bool hasDx = m_ws->hasDx(0);
  for (int bin = 0; bin < m_nBins; bin++) {
    if (m_isCommonBins) {
      file << points0[bin];
    } else // checking for ragged workspace
    {
      file << pointsSpec[bin];
    }
    file << m_sep;
    file << m_ws->y(wsIndex)[bin];

    file << m_sep;
    file << m_ws->e(wsIndex)[bin];
    if (m_writeDX) {
      if (hasDx) {
        file << m_sep;
        file << pointDeltas[bin];
      } else {
        g_log.information("SaveAscii2: WriteXError is requested but there are no Dx data in the workspace");
      }
    }
    file << '\n';
  }
}

/**
 * Converts a comma separated list to a vector of strings
 * Also ensures all strings are valid input
 * @param inputString	:: The user input comma separated string list
 * @return A vector of valid meta data strings
 */
std::vector<std::string> SaveAscii2::stringListToVector(std::string &inputString) {
  const std::vector<std::string> validMetaData{"spectrumnumber", "q", "angle"};
  boost::to_lower(inputString);
  auto stringVector = Kernel::VectorHelper::splitStringIntoVector<std::string>(inputString);
  const auto it = std::find_if(stringVector.cbegin(), stringVector.cend(), [&validMetaData](const auto &input) {
    return std::find(validMetaData.begin(), validMetaData.end(), input) == validMetaData.end();
  });
  if (it != stringVector.cend()) {
    throw std::runtime_error(*it + " is not recognised as a possible input "
                                   "for SpectrumMetaData.\n Valid inputs "
                                   "are: SpectrumNumber, Q, Angle.");
  }

  return stringVector;
}

/**
 * Populate the map with the Q values associated with each spectrum in the
 * workspace
 */
void SaveAscii2::populateQMetaData() {
  std::vector<std::string> qValues;
  const auto nHist = m_ws->getNumberHistograms();
  const auto &spectrumInfo = m_ws->spectrumInfo();
  for (size_t i = 0; i < nHist; i++) {
    double theta(0.0), efixed(0.0);
    if (!spectrumInfo.isMonitor(i)) {
      theta = 0.5 * spectrumInfo.twoTheta(i);
      try {
        std::shared_ptr<const Geometry::IDetector> detector(&spectrumInfo.detector(i), NoDeleting());
        efixed = m_ws->getEFixed(detector);
      } catch (std::runtime_error &) {
        throw;
      }
    } else {
      theta = 0.0;
      efixed = DBL_MIN;
    }
    // Convert to MomentumTransfer
    auto qValue = Kernel::UnitConversion::convertToElasticQ(theta, efixed);
    auto qValueStr = boost::lexical_cast<std::string>(qValue);
    qValues.emplace_back(qValueStr);
  }
  m_metaDataMap["q"] = qValues;
}

/**
 * Populate the map with the SpectrumNumber for each Spectrum in the workspace
 */
void SaveAscii2::populateSpectrumNumberMetaData() {
  std::vector<std::string> spectrumNumbers;
  const size_t nHist = m_ws->getNumberHistograms();
  for (size_t i = 0; i < nHist; i++) {
    const auto specNum = m_ws->getSpectrum(i).getSpectrumNo();
    const auto specNumStr = std::to_string(specNum);
    spectrumNumbers.emplace_back(specNumStr);
  }
  m_metaDataMap["spectrumnumber"] = spectrumNumbers;
}

/**
 * Populate the map with the Angle for each spectrum in the workspace
 */
void SaveAscii2::populateAngleMetaData() {
  std::vector<std::string> angles;
  const size_t nHist = m_ws->getNumberHistograms();
  const auto &spectrumInfo = m_ws->spectrumInfo();
  for (size_t i = 0; i < nHist; i++) {
    const auto two_theta = spectrumInfo.twoTheta(i);
    constexpr double rad2deg = 180. / M_PI;
    const auto angleInDeg = two_theta * rad2deg;
    const auto angleInDegStr = boost::lexical_cast<std::string>(angleInDeg);
    angles.emplace_back(angleInDegStr);
  }
  m_metaDataMap["angle"] = angles;
}

/**
 * Populate all required meta data in the meta data map
 */
void SaveAscii2::populateAllMetaData() {
  for (const auto &metaDataType : m_metaData) {
    if (metaDataType == "spectrumnumber")
      populateSpectrumNumberMetaData();
    if (metaDataType == "q")
      populateQMetaData();
    if (metaDataType == "angle")
      populateAngleMetaData();
  }
}

bool SaveAscii2::findElementInUnorderedStringVector(const std::vector<std::string> &vector, const std::string &toFind) {
  return std::find(vector.cbegin(), vector.cend(), toFind) != vector.cend();
}

void SaveAscii2::writeTableWorkspace(const ITableWorkspace_const_sptr &tws, const std::string &filename,
                                     bool appendToFile, bool writeHeader, int prec, bool scientific,
                                     const std::string &comment) {

  std::ofstream file(filename.c_str(), (appendToFile ? std::ios::app : std::ios::out));

  if (file.bad()) {
    throw Exception::FileError("Unable to create file: ", filename);
  }
  // Set the number precision
  if (prec != EMPTY_INT()) {
    file.precision(prec);
  }
  if (scientific) {
    file << std::scientific;
  }

  const auto columnCount = tws->columnCount();
  if (writeHeader) {
    // write the column names
    file << comment << " ";
    for (size_t colIndex = 0; colIndex < columnCount; colIndex++) {
      file << tws->getColumn(colIndex)->name() << " ";
      if (colIndex < columnCount - 1) {
        file << m_sep << " ";
      }
    }
    file << '\n';
    // write the column types
    file << comment << " ";
    for (size_t colIndex = 0; colIndex < columnCount; colIndex++) {
      file << tws->getColumn(colIndex)->type() << " ";
      if (colIndex < columnCount - 1) {
        file << m_sep << " ";
      }
    }
    file << '\n';
  } else {
    g_log.warning("Please note that files written without headers cannot be "
                  "reloaded back into Mantid with LoadAscii.");
  }

  // write the data
  const auto rowCount = tws->rowCount();
  Progress progress(this, 0.0, 1.0, rowCount);
  for (size_t rowIndex = 0; rowIndex < rowCount; rowIndex++) {
    for (size_t colIndex = 0; colIndex < columnCount; colIndex++) {
      tws->getColumn(colIndex)->print(rowIndex, file);
      if (colIndex < columnCount - 1) {
        file << m_sep;
      }
    }
    file << "\n";
    progress.report();
  }

  file.unsetf(std::ios_base::floatfield);
  file.close();
}

/**
 * Retrieves sample log value and its unit. In case they are not defined they
 * are replaced with 'not defined' meassage and empty string, respectively.
 * @param logName :: The user-defined identifier for sample log
 * @return A pair of strings containing sample log value and its unit
 */
std::pair<std::string, std::string> SaveAscii2::sampleLogValueUnit(const std::string &logName) {
  auto run = m_ws->run();
  // Gets the sample log value
  std::string sampleLogValue = "";
  try {
    sampleLogValue = boost::lexical_cast<std::string>(run.getLogData(logName)->value());
  } catch (Exception::NotFoundError &) {
    g_log.warning("Log " + logName + " not found.");
    sampleLogValue = "Not defined";
  }
  // Gets the sample log unit
  std::string sampleLogUnit = "";
  try {
    sampleLogUnit = boost::lexical_cast<std::string>(run.getLogData(logName)->units());
  } catch (Exception::NotFoundError &) {
    sampleLogUnit = "";
  }
  return std::pair(sampleLogValue, sampleLogUnit);
}

/**
 * Writes the file header containing the user-defined sample logs.
 * @param logList :: A vector of strings containing user-defined identifiers for
 * sample logs
 * @param outputFile :: A reference to the output stream
 */
void SaveAscii2::writeFileHeader(const std::vector<std::string> &logList, std::ofstream &outputFile) {
  for (const auto &logName : logList) {
    const std::pair<std::string, std::string> readLog = sampleLogValueUnit(logName);
    auto logValue = boost::replace_all_copy(readLog.second, ",", ";");
    outputFile << logName << m_sep << readLog.first << m_sep << logValue << '\n';
  }
  outputFile << '\n';
}

void SaveAscii2::write1DHistoCut(const IMDHistoWorkspace_const_sptr &mdws, const std::string &filename,
                                 bool appendToFile, bool writeHeader, int prec, bool scientific,
                                 const std::string &comment) {
  auto dims = mdws->getNonIntegratedDimensions();
  if (dims.size() != 1) {
    throw std::runtime_error("SaveAscii does not support saving multidimentional MDHistoWorkspace, and only supports "
                             "1D MDHistoWorkspaces cuts");
  }

  std::ofstream file(filename.c_str(), (appendToFile ? std::ios::app : std::ios::out));

  if (file.bad()) {
    throw Exception::FileError("Unable to create file: ", filename);
  }

  if (prec != EMPTY_INT()) {
    file.precision(prec);
  }

  if (scientific) {
    file << std::scientific;
  }

  auto lastAlg = mdws->getHistory().lastAlgorithm();
  auto &propsVec = lastAlg->getProperties();

  Mantid::Geometry::IMDDimension_const_sptr dim = dims[0];

  if (writeHeader) {
    // write last algorithm arguments
    file << comment << " {";
    for (size_t i = 0; i < propsVec.size(); i++) {
      file << propsVec[i]->name() << ": " << propsVec[i]->valueAsPrettyStr();
      if (i < propsVec.size() - 1)
        file << m_sep << " ";
    }
    file << " }\n";

    // write columns labels
    auto dimName = dim->getName();
    auto dimUnit = dim->getUnits().latex();

    file << comment << " " << dimName << " " << dimUnit << m_sep << " "
         << "Signal" << m_sep << " "
         << "Error"
         << "\n";
  }

  auto nbins = dim->getNBins();
  auto binWidth = dim->getBinWidth() / 2;
  auto binMax = dim->getMaximum();
  auto binMin = dim->getMinimum();

  auto start = binMin + binWidth;
  auto end = binMax - binWidth;
  auto step = (end - start) / float(nbins - 1);

  auto nPoints = mdws->getNPoints();
  auto signal = mdws->getSignalArray();
  auto error = mdws->getErrorSquaredArray();

  Progress progress(this, 0.0, 1.0, nbins);

  // write data
  for (size_t i = 0; i < nPoints; ++i) {
    file << start + step * float(i) << m_sep << signal[i] << m_sep << std::sqrt(error[i]) << "\n";
    progress.report();
  }

  file.unsetf(std::ios_base::floatfield);
  file.close();

  return;
}
} // namespace Mantid::DataHandling
