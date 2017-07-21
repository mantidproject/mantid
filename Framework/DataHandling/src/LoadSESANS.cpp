#include "MantidDataHandling/LoadSESANS.h"

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Workspace.h"

#include "boost/algorithm/string/predicate.hpp"

#include <algorithm>
#include <fstream>
#include <regex>

namespace Mantid {
namespace DataHandling {

// Register with the AlgorithmFactory
DECLARE_FILELOADER_ALGORITHM(LoadSESANS)

/// Get algorithm's name
const std::string LoadSESANS::name() const {
	return "LoadSESANS";
}

/// Get summary of algorithm
const std::string LoadSESANS::summary() const {
	return "Load a file using the SESANS format";
}

/// Get algorithm's version number
int LoadSESANS::version() const {
	return 1;
}

/// Get algorithm's category
const std::string LoadSESANS::category() const {
	return "DataHandling\\Text";
}

/** Get the confidence that this algorithm can load a file
 * @param descriptor A descriptor for the file
 * @return The confidence level (0 to 100) where 0 indicates this file will not be used
 */
int LoadSESANS::confidence(Kernel::FileDescriptor &descriptor) const {
	return 50; //ARBITRARY - CHANGE THIS
}

/**
 * Initialise the algorithm
 */
void LoadSESANS::init() {
	declareProperty(Kernel::make_unique<API::FileProperty>(
		"Filename", "", API::FileProperty::Load, fileExtensions),
		"Name of the SESANS file to load");
	declareProperty(Kernel::make_unique<API::WorkspaceProperty<>>(
		"OutputWorkspace", "", Kernel::Direction::Output),
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

	// Read in all the header values, and make sure all the mandatory ones are supplied
	consumeHeaders(infile, line, lineNum);
	lineNum++;
	checkMandatoryHeaders();

	// Make sure we haven't reached the end of the file without reading any data
	if (line != "BEGIN_DATA")
		throwFormatError("<EOF>", "Expected \"BEGIN_DATA\" before EOF", lineNum + 1);

	auto columns = consumeData(infile, line, lineNum);

	infile.close();
}

/** Read headers from the input file into the attribute map, until BEGIN_DATA is found
* @param infile Reference to the input file
* @param line Reference to the line currently being processed
* @param lineNum Number of the line currently being processed
* @throw runtime_error If incorrectly formatted headers are found (empty lines are permitted)
*/
void LoadSESANS::consumeHeaders(std::ifstream &infile, std::string &line, int &lineNum) {
	std::pair<std::string, std::string> attr;

	do {
		lineNum++;
		if (!line.empty()) {
			// Split up the line into a key-value pair and add it to our set of attributes
			attr = splitHeader(line, lineNum);
			attributes.insert(attr);
		}
	} while (std::getline(infile, line) && line != "BEGIN_DATA");
}

/** Read numerical data from the file into a map of the form <column name> -> <column data>. Any lines which are badly formed are ignored, and a warning passed to the user
* @param infile Reference to the input file
* @param line Reference to the line of the file currently being processed
* @param lineNum The number of the line being processed
* @return A mapping from column header to vector of strings representing the numbers in that column
* @throw runtime_error If the 4 mandatory columns are not supplied in the file
*/
std::unordered_map<std::string, std::vector<std::string>> LoadSESANS::consumeData(std::ifstream &infile, std::string &line, int &lineNum) {
	std::string numberRegex = "-?\\d+(\\.\\d+)?(E-\\d+)?";

	std::getline(infile, line);
	auto columnHeaders = split(line);

	// Make sure all 4 mandatory columns have been supplied
	for (std::string header : mandatoryColumnHeaders)
		if (std::find(columnHeaders.begin(), columnHeaders.end(), header) == columnHeaders.end())
			throwFormatError(line, "Failed to supply mandatory column header: \"" + header + "\"", lineNum);

	std::regex lineRegex("^\\s*" + repeatAndJoin(numberRegex, "\\s*", columnHeaders.size()));

	// Tokens in a line
	std::vector<std::string> tokens;

	// Map of column name -> column values
	std::unordered_map<std::string, std::vector<std::string>> columns;

	while (std::getline(infile, line)) {
		lineNum++;

		if (std::regex_match(line, lineRegex)) {
			tokens = split(line);

			for (int i = 0; i < tokens.size(); i++)
				columns[columnHeaders[i]].push_back(tokens[i]);
		}
		else {
			g_log.warning("Line " + std::to_string(lineNum) + " discarded, as it was badly formed. Expected " + std::to_string(columnHeaders.size()) + " numbers, but got \"" + line + "\"");
		}
	}
	g_log.information("Loaded " + std::to_string(columns[columnHeaders[0]].size()) + " rows of data");
	return columns;
}

/**
* Split a header into a key-value pair delimited by whitespace, where the first token is the key and the remainder the value
* @param line A string containing the line to split
* @param lineNum The number of the line in the file, used for error messaging
* @return A key-value pair
* @throw runtime_error If the line contains less than two tokens
*/
std::pair<std::string, std::string> LoadSESANS::splitHeader(const std::string &line, const int &lineNum) {
	std::pair<std::string, std::string> attribute;
	auto i = line.begin();

	// Discard leading whitespace
	i = find_if(i, line.end(), &notSpace);
	// Find the end of the first word
	auto j = find_if(i, line.end(), &space);

	if (j == line.end())
		throwFormatError(line, "Expected key-value pair", lineNum);

	attribute.first = std::string(i, j);

	// Find start of second word
	i = find_if(j, line.end(), notSpace);
	if (i == line.end())
		throwFormatError(line, "Expected key-value pair", lineNum);

	// Grab from there to the end of the line
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
	std::string output = "Badly formed line at line " + std::to_string(lineNum) + "\n\n" +
		"\t" + line + "\n\n" + message;
	g_log.error(output);
	throw std::runtime_error(output);
}

/** Make sure that all mandatory headers are supplied in the file
* @throw runtime_error If any other the mandatory headers are missing
*/
void LoadSESANS::checkMandatoryHeaders() {
	for (std::string attr : mandatoryAttributes) {
		if (attributes.find(attr) == attributes.end()) {
			std::string err = "Failed to supply parameter: \"" + attr + "\"";
			g_log.error(err);
			throw std::runtime_error(err);
		}
	}
}

/** Is a character whitespace (here considered space or tab)?
 * @param c The character
 * @return Whether it is whitespace
 */
bool LoadSESANS::space(const char &c) {
	return c == ' ' || c == '\t';
}

/** Is a character not whitespace (here considered space or tab)?
* @param c The character
* @return Whether it is not whitespace
*/
bool LoadSESANS::notSpace(const char &c) {
	return !space(c);
}

/** Split a string
 * @param str The string to split
 * @param delim The delimiter
 * @return Vector of string segments
 */
std::vector<std::string> LoadSESANS::split(const std::string &str, const char &delim) {
	std::vector<std::string> result;

	auto i = str.begin();
	i = find_if(i, str.end(), &notSpace);

	auto j = find_if(i, str.end(), &space);

	while (i != str.end()) {
		result.push_back(std::string(i, j));
		i = find_if(j, str.end(), &notSpace);
		j = find_if(i, str.end(), &space);
	}

	return result;
}

/** Repeat a string n times, delimited by another string. eg. repeatAndJoin("a", "b", 3) == "ababab"
 * @param str The string to repeat
 * @param delim The delimiter
 * @param n The number of times to repeat
 * @return The repeated string
*/
std::string LoadSESANS::repeatAndJoin(const std::string &str, const std::string &delim, int n) {
	std::string result = "";
	for (int i = 0; i < n; i++) {
		result += str + delim;
	}
	return result + str;
}

} // namespace DataHandling
} // namespace Mantid
