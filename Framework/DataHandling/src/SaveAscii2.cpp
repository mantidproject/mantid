// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/SaveAscii2.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
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

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveAscii2)

using namespace Kernel;
using namespace API;

/// Empty constructor
SaveAscii2::SaveAscii2()
    : m_separatorIndex(), m_nBins(0), m_sep(), m_writeDX(false),
      m_writeID(false), m_isCommonBins(false), m_writeSpectrumAxisValue(false),
      m_ws() {}

/// Initialisation method.
void SaveAscii2::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<Workspace>>("InputWorkspace", "",
                                                     Direction::Input),
      "The name of the workspace containing the data you want to save to a "
      "Ascii file.");

  const std::vector<std::string> asciiExts{".dat", ".txt", ".csv"};
  declareProperty(std::make_unique<FileProperty>("Filename", "",
                                                 FileProperty::Save, asciiExts),
                  "The filename of the output Ascii file.");

  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(1);
  auto mustBeZeroGreater = boost::make_shared<BoundedValidator<int>>();
  mustBeZeroGreater->setLower(0);
  declareProperty(
      "WorkspaceIndexMin", EMPTY_INT(), mustBeZeroGreater,
      "The starting workspace index. Ignored for Table Workspaces.");
  declareProperty("WorkspaceIndexMax", EMPTY_INT(), mustBeZeroGreater,
                  "The ending workspace index. Ignored for Table Workspaces.");
  declareProperty(
      std::make_unique<ArrayProperty<int>>("SpectrumList"),
      "List of workspace indices to save. Ignored for Table Workspaces.");
  declareProperty("Precision", EMPTY_INT(), mustBePositive,
                  "Precision of output double values.");
  declareProperty("ScientificFormat", false,
                  "If true, the values will be "
                  "written to the file in "
                  "scientific notation.");
  declareProperty("WriteXError", false,
                  "If true, the error on X will be written as the fourth "
                  "column. Ignored for Table Workspaces.");
  declareProperty(
      "WriteSpectrumID", true,
      "If false, the spectrum No will not be written for "
      "single-spectrum workspaces. "
      "It is always written for workspaces with multiple spectra, "
      "unless spectrum axis value is written. Ignored for Table Workspaces.");

  declareProperty("CommentIndicator", "#",
                  "Character(s) to put in front of comment lines.");

  // For the ListValidator
  std::string spacers[6][2] = {
      {"CSV", ","},   {"Tab", "\t"},      {"Space", " "},
      {"Colon", ":"}, {"SemiColon", ";"}, {"UserDefined", "UserDefined"}};
  std::vector<std::string> sepOptions;
  for (auto &spacer : spacers) {
    std::string option = spacer[0];
    m_separatorIndex.insert(
        std::pair<std::string, std::string>(option, spacer[1]));
    sepOptions.emplace_back(option);
  }

  declareProperty("Separator", "CSV",
                  boost::make_shared<StringListValidator>(sepOptions),
                  "The separator between data columns in the data file. The "
                  "possible values are \"CSV\", \"Tab\", "
                  "\"Space\", \"SemiColon\", \"Colon\" or \"UserDefined\".");

  declareProperty(
      std::make_unique<PropertyWithValue<std::string>>("CustomSeparator", "",
                                                       Direction::Input),
      "If present, will override any specified choice given to Separator.");

  setPropertySettings("CustomSeparator",
                      std::make_unique<VisibleWhenProperty>(
                          "Separator", IS_EQUAL_TO, "UserDefined"));
  getPointerToProperty("CustomSeparator")->setAutoTrim(false);
  declareProperty("ColumnHeader", true,
                  "If true, put column headers into file. ");

  declareProperty("SpectrumMetaData", "",
                  "A comma separated list that defines data that describes "
                  "each spectrum in a workspace. The valid options for this "
                  "are: SpectrumNumber,Q,Angle. Ignored for Table Workspaces.");

  declareProperty(
      "AppendToFile", false,
      "If true, don't overwrite the file. Append to the end of it. ");

  declareProperty("RaggedWorkspace", true,
                  "If true, ensure that more than one xspectra is used. "
                  "Ignored for Table Workspaces."); // in testing

  declareProperty("WriteSpectrumAxisValue", false,
                  "Write the spectrum axis value if requested. Ignored for "
                  "Table Workspaces.");
}

/**
 *   Executes the algorithm.
 */
void SaveAscii2::exec() {
  // Get the workspace
  Workspace_const_sptr ws = getProperty("InputWorkspace");
  m_ws = boost::dynamic_pointer_cast<const MatrixWorkspace>(ws);
  ITableWorkspace_const_sptr tws =
      boost::dynamic_pointer_cast<const ITableWorkspace>(ws);

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

  if (tws) {
    writeTableWorkspace(tws, filename, appendToFile, writeHeader, prec,
                        scientific, comment);
    // return here as the rest of the class is all about matrix workspace saving
    return;
  }

  if (!m_ws) {
    throw std::runtime_error(
        "SaveAscii does not now how to save this workspace type, " +
        ws->getName());
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
    auto containsSpectrumNumber =
        findElementInUnorderedStringVector(m_metaData, "spectrumnumber");
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
    auto containsSpectrumNumber =
        findElementInUnorderedStringVector(m_metaData, "spectrumnumber");
    if (!containsSpectrumNumber) {
      auto firstIter = m_metaData.begin();
      m_metaData.insert(firstIter, "spectrumnumber");
    }
  }

  if (m_writeSpectrumAxisValue) {
    auto spectrumAxis = m_ws->getAxis(1);
    if (dynamic_cast<BinEdgeAxis *>(spectrumAxis)) {
      m_axisProxy =
          std::make_unique<AxisHelper::BinEdgeAxisProxy>(spectrumAxis);
    } else {
      m_axisProxy = std::make_unique<AxisHelper::AxisProxy>(spectrumAxis);
    }
  }

  // Check whether we need to write the fourth column
  if (!m_ws->hasDx(0) && m_writeDX) {
    throw std::runtime_error(
        "x data errors have been requested but do not exist.");
  }

  // e + and - are included as they're part of the scientific notation
  if (!boost::regex_match(m_sep.begin(), m_sep.end(),
                          boost::regex("[^0-9e+-]+", boost::regex::perl))) {
    throw std::invalid_argument("Separators cannot contain numeric characters, "
                                "plus signs, hyphens or 'e'");
  }

  if (comment.at(0) == m_sep.at(0) ||
      !boost::regex_match(
          comment.begin(), comment.end(),
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
    if (spec_min >= nSpectra || spec_max >= nSpectra || spec_min < 0 ||
        spec_max < 0 || spec_min > spec_max) {
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
  if (!idx.empty()) {
    nSpectra = static_cast<int>(idx.size());
  }

  if (m_nBins == 0 || nSpectra == 0) {
    throw std::runtime_error("Trying to save an empty workspace");
  }
  std::ofstream file(filename.c_str(),
                     (appendToFile ? std::ios::app : std::ios::out));

  if (!file) {
    g_log.error("Unable to create file: " + filename);
    throw Exception::FileError("Unable to create file: ", filename);
  }
  // Set the number precision
  if (prec != EMPTY_INT()) {
    file.precision(prec);
  }
  if (scientific) {
    file << std::scientific;
  }
  if (writeHeader) {
    file << comment << " X " << m_sep << " Y " << m_sep << " E";
    if (m_writeDX) {
      file << " " << m_sep << " DX";
    }
    file << '\n';
  }
  // populate the meta data map
  if (!m_metaData.empty()) {
    populateAllMetaData();
  }
  if (idx.empty()) {
    Progress progress(this, 0.0, 1.0, nSpectra);
    for (int i = 0; i < nSpectra; i++) {
      writeSpectrum(i, file);
      progress.report();
    }
  } else {
    Progress progress(this, 0.0, 1.0, idx.size());
    for (int i : idx) {
      writeSpectrum(i, file);
      progress.report();
    }
  }

  file.unsetf(std::ios_base::floatfield);
  file.close();
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
      file << m_sep;
      file << pointDeltas[bin];
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
std::vector<std::string>
SaveAscii2::stringListToVector(std::string &inputString) {
  const std::vector<std::string> validMetaData{"spectrumnumber", "q", "angle"};
  boost::to_lower(inputString);
  auto stringVector =
      Kernel::VectorHelper::splitStringIntoVector<std::string>(inputString);
  for (const auto &input : stringVector) {
    if (std::find(validMetaData.begin(), validMetaData.end(), input) ==
        validMetaData.end()) {
      throw std::runtime_error(input + " is not recognised as a possible input "
                                       "for SpectrumMetaData.\n Valid inputs "
                                       "are: SpectrumNumber, Q, Angle.");
    }
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
        boost::shared_ptr<const Geometry::IDetector> detector(
            &spectrumInfo.detector(i), NoDeleting());
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

bool SaveAscii2::findElementInUnorderedStringVector(
    const std::vector<std::string> &vector, const std::string &toFind) {
  return std::find(vector.cbegin(), vector.cend(), toFind) != vector.cend();
}

void SaveAscii2::writeTableWorkspace(ITableWorkspace_const_sptr tws,
                                     const std::string &filename,
                                     bool appendToFile, bool writeHeader,
                                     int prec, bool scientific,
                                     const std::string &comment) {

  std::ofstream file(filename.c_str(),
                     (appendToFile ? std::ios::app : std::ios::out));

  if (!file) {
    g_log.error("Unable to create file: " + filename);
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

} // namespace DataHandling
} // namespace Mantid
