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

	if (!infile) {
		g_log.error("Unable to open file " + filename);
		throw Kernel::Exception::FileError("Unable to open file ", filename);
	}

	g_log.information() << "Opened file \"" << filename << "\" for reading\n";

	int lineNum = 0;
	std::string line;
	std::getline(infile, line);
	lineNum++;

	// First line must be FileFormatVersion:
	if (!boost::starts_with(line, "FileFormatVersion"))
		throwFormatError(line, "File must begin by providing FileFormatVersion", lineNum);

	consumeHeaders(infile, line, lineNum);
}

std::pair<std::string, std::string> LoadSESANS::splitHeader(const std::string &line, const int &lineNum) {
	std::pair<std::string, std::string> attribute;
	auto i = line.begin();

	// Discard leading whitespace
	i = find_if(i, line.end(), notSpace);
	// Find the end of the first word
	auto j = find_if(i, line.end(), space);
	
	if (j == line.end())
		throwFormatError(line, "Expected key-value pair", lineNum);
	attribute.first = std::string(i, j);

	//Find start and end of second word
	i = find_if(j, line.end(), notSpace);
	if (i == line.end())
		throwFormatError(line, "Expected key-value pair", lineNum);
	j = find_if(i, line.end(), space);
	attribute.second = std::string(i, j);

	i = find_if(j, line.end(), notSpace);
	if (i != line.end())
		g_log.warning("Too many values supplied at line " + std::to_string(lineNum) +
			". Discarded all but the first 2");
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

	while (line != "BEGIN_DATA") {
		if (std::getline(infile, line)) {
			lineNum++;
			attr = splitHeader(line, lineNum);
			attributes[attr.first] = attr.second;
		}
		else {
			throwFormatError("EOF", "Expected \"BEGIN_DATA\" before EOF", lineNum);
		}
	}
}

} // namespace DataHandling
} // namespace Mantid
