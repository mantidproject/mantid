// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadSESANS.h"

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceFactory.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/regex.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>

namespace { // Anonymous namespace for helper functions

/** Is a character whitespace (here considered space or tab)?
 * @param c The character
 * @return Whether it is whitespace
 */
bool space(const char &c) { return isspace(c) != 0; }

/** Is a character not whitespace (here considered space or tab)?
 * @param c The character
 * @return Whether it is not whitespace
 */
bool notSpace(const char &c) { return !space(c); }

/** Is a string all whitespace?
 * @param str The string to test
 * @return Whether every character is whitespace
 */
bool allSpaces(const std::string &str) { return std::all_of(str.begin(), str.end(), space); }

/** Repeat a string n times, delimited by another string. eg. repeatAndJoin("a",
 * "b", 3) == "ababa"
 * @param str The string to repeat
 * @param delim The delimiter
 * @param n The number of times to repeat
 * @return The repeated string
 */
std::string repeatAndJoin(const std::string &str, const std::string &delim, const int &n) {
  std::string result = "";
  for (int i = 0; i < n - 1; i++) {
    result += str + delim;
  }
  return result + str;
}
} // Anonymous namespace

namespace Mantid::DataHandling {

// Register with the AlgorithmFactory
DECLARE_FILELOADER_ALGORITHM(LoadSESANS)

/// Get algorithm's name
const std::string LoadSESANS::name() const { return "LoadSESANS"; }

/// Get summary of algorithm
const std::string LoadSESANS::summary() const { return "Load a file using the SESANS format"; }

/// Get algorithm's version number
int LoadSESANS::version() const { return 1; }

/// Get algorithm's category
const std::string LoadSESANS::category() const { return "DataHandling\\Text"; }

/** Get the confidence that this algorithm can load a file
 * @param descriptor A descriptor for the file
 * @return The confidence level (0 to 100) where 0 indicates this algorithm will
 * not be used
 */
int LoadSESANS::confidence(Kernel::FileDescriptor &descriptor) const {
  // Check we're looking at a text-based file
  if (!descriptor.isAscii())
    return 0;

  // If the file has a SESANS extension
  if (std::find(m_fileExtensions.begin(), m_fileExtensions.end(), descriptor.extension()) != m_fileExtensions.end())
    return 70;

  // Nothing was obviously right or wrong, so we'll have to dig around a bit in
  // the file

  auto &file = descriptor.data();
  std::string line;

  // First line should be FileFormatVersion
  std::getline(file, line);
  bool ffvFound = boost::starts_with(line, "FileFormatVersion");

  // Next few lines should be key-value pairs
  boost::regex kvPair(R"([\w_]+\s+[\w\d\.\-]+(\s+[\w\d\.\-\$]+)*)");
  int kvPairsFound = 0;

  for (int i = 0; i < 3 && !line.empty(); i++) {
    if (boost::regex_match(line, kvPair)) {
      kvPairsFound++;
    }
    std::getline(file, line);
  }

  // There are 13 mandatory key-value pairs. If there are 11 found, a couple may
  // just have been missed off, but if there are fewer than we're probably
  // looking at something else
  if (kvPairsFound < 10)
    return 0;

  // Next non-blank line
  while (std::getline(file, line) && line.empty())
    ;

  bool beginFound = line == m_beginData;

  // Return something which takes us above other ASCII formats, as long as
  // FileFormatVersion and BEGIN_DATA were found
  return 15 + 3 * ffvFound + 3 * beginFound;
}

/**
 * Initialise the algorithm
 */
void LoadSESANS::init() {
  declareProperty(std::make_unique<API::FileProperty>("Filename", "", API::FileProperty::Load, m_fileExtensions),
                  "Name of the SESANS file to load");
  declareProperty(std::make_unique<API::WorkspaceProperty<>>("OutputWorkspace", "", Kernel::Direction::Output),
                  "The name to use for the output workspace");
}

/**
 * Execute the algorithm
 */
void LoadSESANS::exec() {
  std::string filename = getPropertyValue("Filename");
  std::ifstream infile(filename);

  // Check file is readable
  if (!infile) {
    g_log.error("Unable to open file " + filename);
    throw Kernel::Exception::FileError("Unable to open file ", filename);
  }

  g_log.information() << "Opened file \"" << filename << "\" for reading\n";

  int lineNum = 0;
  std::string line;
  std::getline(infile, line);

  // First line must be FileFormatVersion:
  if (!boost::starts_with(line, "FileFormatVersion"))
    throwFormatError(line, "File must begin by providing FileFormatVersion", lineNum);

  // Read in all the header values, and make sure all the mandatory ones are
  // supplied
  AttributeMap attributes = consumeHeaders(infile, line, lineNum);
  lineNum++;
  checkMandatoryHeaders(attributes);

  // Make sure we haven't reached the end of the file without reading any data
  if (!boost::starts_with(line, m_beginData))
    throwFormatError("<EOF>", "Expected \"" + m_beginData + "\" before EOF", lineNum + 1);

  // Read file columns into a map - now we can get rid of the file
  ColumnMap columns = consumeData(infile, line, lineNum);
  infile.close();

  // Make a workspace from the columns and set it as the output
  API::MatrixWorkspace_sptr newWorkspace = makeWorkspace(columns);

  newWorkspace->setTitle(attributes["DataFileTitle"]);
  newWorkspace->mutableSample().setName(attributes["Sample"]);
  newWorkspace->mutableSample().setThickness(std::stod(attributes["Thickness"]));

  setProperty("OutputWorkspace", newWorkspace);
}

/** Read headers from the input file into the attribute map, until BEGIN_DATA is
 * found
 * @param infile Reference to the input file
 * @param line Reference to the line currently being processed
 * @param lineNum Number of the line currently being processed
 * @return Map of attributes read
 * @throw runtime_error If incorrectly formatted headers are found (empty lines
 * are permitted)
 */
AttributeMap LoadSESANS::consumeHeaders(std::ifstream &infile, std::string &line, int &lineNum) {
  AttributeMap attributes;
  std::pair<std::string, std::string> attr;

  do {
    lineNum++;
    if (!allSpaces(line)) {
      // Split up the line into a key-value pair and add it to our set of
      // attributes
      attr = splitHeader(line, lineNum);
      attributes.insert(attr);
    }
  } while (std::getline(infile, line) && !boost::starts_with(line, m_beginData));

  return attributes;
}

/** Read numerical data from the file into a map of the form [column name] ->
 * [column data]. Any lines which are badly formed are ignored, and a warning
 * passed to the user
 * @param infile Reference to the input file
 * @param line Reference to the line of the file currently being processed
 * @param lineNum The number of the line being processed
 * @return A mapping from column header to vector of strings representing the
 * numbers in that column
 * @throw runtime_error If the 4 mandatory columns are not supplied in the file
 */
ColumnMap LoadSESANS::consumeData(std::ifstream &infile, std::string &line, int &lineNum) {
  std::getline(infile, line);
  std::vector<std::string> columnHeaders;
  boost::trim(line);
  boost::split(columnHeaders, line, isspace, boost::token_compress_on);

  // Make sure all 4 mandatory columns have been supplied
  for (const std::string &header : m_mandatoryColumnHeaders)
    if (std::find(columnHeaders.begin(), columnHeaders.end(), header) == columnHeaders.end())
      throwFormatError(line, "Failed to supply mandatory column header: \"" + header + "\"", lineNum);

  std::string numberRegex = R"((-?\d+(\.\d+)?([Ee][-\+]?\d+)?))";
  // static_cast is safe as realistically our file is never going to have enough
  // columns to overflow
  std::string rawRegex = "^\\s*" + repeatAndJoin(numberRegex, "\\s+", static_cast<int>(columnHeaders.size())) + "\\s*$";
  boost::regex lineRegex(rawRegex);

  // Map of column name -> column values
  ColumnMap columns;

  while (std::getline(infile, line)) {
    lineNum++;

    if (boost::regex_match(line, lineRegex)) {
      // Tokens in a line
      std::vector<std::string> tokens;

      boost::trim(line);
      boost::split(tokens, line, isspace, boost::token_compress_on);

      for (size_t i = 0; i < tokens.size(); i++)
        columns[columnHeaders[i]].emplace_back(std::stod(tokens[i]));
    } else {
      g_log.warning("Line " + std::to_string(lineNum) + " discarded, as it was badly formed. Expected " +
                    std::to_string(columnHeaders.size()) + " numbers, but got \"" + line + "\"");
    }
  }
  g_log.information("Loaded " + std::to_string(columns[columnHeaders[0]].size()) + " rows of data");
  return columns;
}

/**
 * Split a header into a key-value pair delimited by whitespace, where the first
 * token is the key and the remainder the value
 * @param untrimmedLine A string containing the line to split, possibly with
 * whitespace at each end
 * @param lineNum The number of the line in the file, used for error messaging
 * @return A key-value pair
 * @throw runtime_error If the line contains less than two tokens
 */
std::pair<std::string, std::string> LoadSESANS::splitHeader(const std::string &untrimmedLine, const int &lineNum) {
  std::pair<std::string, std::string> attribute;
  const auto &line = boost::trim_copy(untrimmedLine);

  auto i = line.begin();
  // Find the end of the first word
  auto j = find_if(i, line.end(), space);

  if (j == line.end())
    throwFormatError(line, "Expected key-value pair", lineNum);

  attribute.first = std::string(i, j);

  // Find start of the second word
  i = find_if(j, line.end(), notSpace);
  if (i == line.end())
    throwFormatError(line, "Expected key-value pair", lineNum);

  // Grab from start of second word to the end of the line
  attribute.second = std::string(i, line.end());

  return attribute;
}

/** Helper function to throw an error relating to the format of the file
 * @param line The line where a format error was found
 * @param message A message detailing the reason for the error
 * @param lineNum The number of the line
 * @throw runtime_error Under all circumstances
 */
void LoadSESANS::throwFormatError(const std::string &line, const std::string &message, const int &lineNum) {
  std::string output = "Badly formed line at line " + std::to_string(lineNum) + ": \"" + line + "\"\n(" + message + ")";
  g_log.error(output);
  throw std::runtime_error(output);
}

/** Make sure that all mandatory headers are supplied in the file
 * @throw runtime_error If any other the mandatory headers are missing
 */
void LoadSESANS::checkMandatoryHeaders(const AttributeMap &attributes) {
  const auto it = std::find_if(m_mandatoryAttributes.cbegin(), m_mandatoryAttributes.cend(),
                               [&attributes](const auto &attr) { return !attributes.count(attr); });
  if (it != m_mandatoryAttributes.cend()) {
    std::string err = "Failed to supply parameter: \"" + *it + "\"";
    g_log.error(err);
    throw std::runtime_error(err);
  }
}

/** Create a new workspace with the columns read from the file
 * @param columns Data columns from input file
 * @return A workspace with the corresponding data
 */
API::MatrixWorkspace_sptr LoadSESANS::makeWorkspace(ColumnMap columns) {
  size_t histogramLength = columns[m_spinEchoLength].size();
  API::MatrixWorkspace_sptr newWorkspace =
      API::WorkspaceFactory::Instance().create("Workspace2D", 1, histogramLength, histogramLength);

  const auto &xValues = columns[m_spinEchoLength];
  const auto &yValues = columns[m_depolarisation];
  const auto &eValues = columns[m_depolarisationError];

  auto &dataX = newWorkspace->mutableX(0);
  auto &dataY = newWorkspace->mutableY(0);
  auto &dataE = newWorkspace->mutableE(0);

  for (size_t i = 0; i < histogramLength; i++) {
    dataX[i] = xValues[i];
    dataY[i] = yValues[i];
    dataE[i] = eValues[i];
  }

  return newWorkspace;
}

} // namespace Mantid::DataHandling
