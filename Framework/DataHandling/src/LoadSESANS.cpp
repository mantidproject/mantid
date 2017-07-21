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

DECLARE_FILELOADER_ALGORITHM(LoadSESANS)

const std::string LoadSESANS::name() const {
	return "LoadSESANS";
}

const std::string LoadSESANS::summary() const {
	return "Load a file using the SESANS format";
}

int LoadSESANS::version() const {
	return 1;
}

const std::string LoadSESANS::category() const {
	return "DataHandling\\Text";
}

int LoadSESANS::confidence(Kernel::FileDescriptor &descriptor) const {
	return 50; //ARBITRARY - CHANGE THIS
}

void LoadSESANS::init() {
	declareProperty(Kernel::make_unique<API::FileProperty>(
		"Filename", "", API::FileProperty::Load, fileExtensions),
		"Name of the SESANS file to load");
	declareProperty(Kernel::make_unique<API::WorkspaceProperty<>>(
		"OutputWorkspace", "", Kernel::Direction::Output),
		"The name to use for the output workspace");
}

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

bool LoadSESANS::space(const char &c) {
	return c == ' ' || c == '\t';
}

bool LoadSESANS::notSpace(const char &c) {
	return !space(c);
}


void LoadSESANS::throwFormatError(const std::string &line, const std::string &message, const int &lineNum) {
	std::string output = "Badly formed line at line " + std::to_string(lineNum) + "\n\n" +
		"\t" + line + "\n\n" + message;
	g_log.error(output);
	throw std::runtime_error(output);
}

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
