/*WIKI*
 TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
 *WIKI*/

#include "MantidDataHandling/LoadILLAsciiHelper.h"

#include <iostream>
#include <fstream>

#include <vector>
#include <string>
#include <map>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/predicate.hpp>


namespace Mantid {
namespace DataHandling {

ILLParser::ILLParser(const std::string &filename) {
	fin.open(filename);
}

ILLParser::~ILLParser() {
	if (fin != NULL) {
		fin.close();
	}
}

/**
 * Main function
 */
void ILLParser::startParsing() {
	std::string line;
	while (std::getline(fin, line)) {
		if (line.find(
				"RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR")
				!= std::string::npos) {
			parseFieldR();
		} else if (line.find(
				"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA")
				!= std::string::npos) {
			parseFieldA();
		} else if (line.find(
				"IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII")
				!= std::string::npos) {
			parseFieldNumeric(header, intWith);
		} else if (line.find(
				"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF")
				!= std::string::npos) {
			parseFieldNumeric(header, floatWith);
		} else if (line.find(
				"SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS")
				!= std::string::npos) {
			startParseSpectra();
		}
	}
}

/**
 *
 */
std::string ILLParser::getInstrumentName(){
	std::string instrumentKeyword("Inst"), instrumentName("");
	if (header.size()  > 0 )  {
		for (auto it = header.begin(); it != header.end(); ++it) {
			if (boost::starts_with(it->first , instrumentKeyword)){
				instrumentName = it->second.substr(0,instrumentKeyword.size());
				boost::algorithm::erase_all(instrumentName, " ");
			}
		}
	}
	return instrumentName;
}

/**
 * Parse fields of type R and keep them in the header
 */
void ILLParser::parseFieldR() {
	std::string line;
	std::getline(fin, line);
	std::vector<std::string> parsedLineFields = splitLineInFixedWithFields(line,
			intWith);
	header["NRUN"] = parsedLineFields[0];
	header["NTEXT"] = parsedLineFields[1];
	header["NVERS"] = parsedLineFields[2];
}

/**
 * Parse fields of type A
 */
void ILLParser::parseFieldA() {
	std::string line;
	std::getline(fin, line);
	std::vector<std::string> parsedLineFields = splitLineInFixedWithFields(line,
			intWith);
	// I'm not using this for now
	//int numberOfCharsToRead = evaluate<int>(parsedLineFields[0]);
	int numberOfLinesToRead = evaluate<int>(parsedLineFields[1]);
	std::string key;
	std::string value("");
	std::getline(fin, key);
	for (int i = 0; i < numberOfLinesToRead; i++) {
		std::getline(fin, line);
		value += line;
	}
	header[key] = value;
}

/**
 * Parses a field of numeric type and puts the result in a map
 */
void ILLParser::parseFieldNumeric(std::map<std::string, std::string> &header,
		int fieldWith) {
	std::string line;
	std::getline(fin, line);
	int nNumericFields = -1, nTextLines = -1;
	sscanf(line.c_str(), "%d %d", &nNumericFields, &nTextLines);

	std::vector<std::string> keys(nNumericFields);
	std::vector<std::string> values(nNumericFields);
	size_t index = 0;
	for (int i = 0; i < nTextLines; i++) {
		std::getline(fin, line);
		std::vector<std::string> s = splitLineInFixedWithFields(line,
				fieldWith);
		for (auto it = s.begin(); it != s.end(); ++it) {
			keys[index] = *it;
			index += 1;
		}
	}
	// parse numeric fields
	size_t pos = 0;
	index = 0;
	while (pos < static_cast<size_t>(nNumericFields)) {
		std::getline(fin, line);
		std::vector<std::string> s = splitLineInFixedWithFields(line,
				fieldWith);
		pos += s.size();
		for (auto it = s.begin(); it != s.end(); ++it) {
			values[index] = *it;
			index += 1;
		}
	}

	std::vector<std::string>::const_iterator iKey;
	std::vector<std::string>::const_iterator iValue;
	for (iKey = keys.begin(), iValue = values.begin();
			iKey < keys.end() && iValue < values.end(); ++iKey, ++iValue) {
		if (*iValue != "" && *iKey != "") {
			header[*iKey] = *iValue;
		}
	}

}

/**
 * Parses the spectrum
 */
std::vector<int> ILLParser::parseFieldISpec(int fieldWith) {
	std::string line;
	std::getline(fin, line);
	int nSpectra;
	sscanf(line.c_str(), "%d", &nSpectra);
	std::vector<int> spectrumValues(nSpectra);

	int nSpectraRead = 0, index = 0;
	while (nSpectraRead < nSpectra) {
		std::getline(fin, line);
		std::vector<std::string> s = splitLineInFixedWithFields(line,
				fieldWith);
		nSpectraRead += static_cast<int>(s.size());
		for (auto it = s.begin(); it != s.end(); ++it) {
			sscanf(it->c_str(), "%d", &spectrumValues[index]);
			index += 1;
		}
	}
	return spectrumValues;
}

/**
 * Shows contents
 */
void ILLParser::showHeader() {
	std::cout << "* Global header" << '\n';
	for (auto it = header.begin(); it != header.end(); ++it)
		std::cout << it->first << " => " << it->second << '\n';

	std::cout << "* Spectrum header" << '\n';
	int i = 0;
	std::vector<std::map<std::string, std::string> >::const_iterator s;
	for (s = spectraHeaders.begin(); s != spectraHeaders.end(); ++s) {
		std::cout << "** Spectrum i : " << i << '\n';
		std::map<std::string, std::string>::const_iterator it;
		for (it = s->begin(); it != s->end(); ++it)
			std::cout << it->first << " => " << it->second << ',';
		std::cout << std::endl;
		i++;
	}

	std::cout << "* Spectrum list" << '\n';
	std::vector<std::vector<int> >::const_iterator l;
	for (l = spectraList.begin(); l != spectraList.end(); ++l) {
		std::cout << "From " << (*l)[0] << " to " << (*l)[l->size() - 1]
				<< " => " << l->size() << '\n';

	}

}

void ILLParser::startParseSpectra() {
	std::string line;
	std::getline(fin, line);
	while (std::getline(fin, line)) {
		if (line.find(
				"IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII")
				!= std::string::npos) {
			spectraList.push_back(parseFieldISpec());

		} else if (line.find(
				"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF")
				!= std::string::npos) {
			spectraHeaders.push_back(std::map<std::string, std::string>());
			parseFieldNumeric(spectraHeaders.back(), floatWith);
		} else if (line.find(
				"SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS")
				!= std::string::npos) {
			std::getline(fin, line);
		}
	}
}

/**
 * Splits a line in fixed length fields.
 */
std::vector<std::string> ILLParser::splitLineInFixedWithFields(
		const std::string &s, int fieldWidth, int lineWitdh) {
	size_t outVecSize = lineWitdh / fieldWidth;
	std::vector<std::string> outVec(outVecSize);
	size_t pos = 0, posInVec = 0;
	while (pos + fieldWidth <= s.length()) {
		std::string subs = s.substr(pos, fieldWidth);
		if (subs.find_first_not_of(' ') != std::string::npos) {
			// not empty substring
			outVec[posInVec] = subs;
			posInVec += 1;
		} else {
			// delete not necessary entries
			outVec.erase(outVec.begin() + posInVec);
		}
		pos += fieldWidth;
	}
	return outVec;
}

template<typename T>
T ILLParser::evaluate(std::string field) {
	boost::algorithm::erase_all(field, " ");
	//std::cout << subs << std::endl;
	T value;
	try {
		value = boost::lexical_cast<T>(field);
	} catch (boost::bad_lexical_cast &e) {
		throw e;
	}
	return value;
}

} // namespace DataHandling
} // namespace Mantid
