#include "MantidAPI/Algorithm.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataHandling/LoadSESANS.h"

#include "boost/algorithm/string/predicate.hpp"
#include <algorithm>
#include <fstream>

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

	infile.close();
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

/** Make sure that all mandatory headers are supplied in the file
 * @throw runtime_error If any other the mandatory headers are missing
 */
void LoadSESANS::checkMandatoryHeaders(){
	for (std::string attr : mandatoryAttributes) {
		if (attributes.find(attr) == attributes.end()) {
			std::string err = "Failed to supply parameter: \"" + attr + "\"";
			g_log.error(err);
			throw std::runtime_error(err);
		}
	}
}

} // namespace DataHandling
} // namespace Mantid
