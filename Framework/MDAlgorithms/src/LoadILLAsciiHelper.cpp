/*WIKI*

 This file contains a class responsible for parsing the ILL ASCII data - aka
'Ron Ghosh' format.

 NOTE: To date only tested for D2B!
 The format may very among the other ILL instruments!

 The data format is fully described here:
 http://www.ill.eu/instruments-support/computing-for-science/data-analysis/raw-data/

 The file format is the following:

Keys, data and text and are written in 80 character fixed length strings (data
following the V descriptor have
variable length).
A key field signifies a certain type of data field follows, with information on
the size of the following field, and how
much text (if any) is present describing the field of data.
The text (if present) then follows (new feature).
The next records then contain the data.
There then follows another key record for the next data field.
The next record contains information on the size of the following field, and how
much text (if any) is present
describing the field of data. etc.,
Key fields
These identifying fields consist of two 80 character strings with a fixed
format. The first is completely filled with one of
the five key letters (R, S, A, F,I,J or V), written with the Fortran format
(80A1); the second contains up to 10 integers
in Fortran format (10I8). The first record can always be read using the A1
format and checked before any attempt is
made to read the following integers. These integers contain control information.
The seven key types are described below:
RRRRRRRRRR..			..RRR	(80A1)
NRUN	NTEXT	NVERS				(10I8)

NRUN	is the run number (numor ) for the data following
NTEXT	is the number of lines of descriptive text which follow
NVERS	is the version of the data (modified as data structure changes)

SSSSSSSSSS..	...		..SSS	(80A1)
ISPEC	NREST	NTOT	NRUN	NTEXT	NPARS	(10I8)

ISPEC	is the following sub-spectrum number
NREST	is the number of subspectra remaining after ISPEC
NTOT	is the total number of subspectra in the run
NRUN	is the current run number
NTEXT	is the number of lines of descriptive text
NPARS	is the number of parameter sections (F, I etc, preceding the
        counts data), typically for step-scanning multi-detector
        instruments where additional information is stored at each step

AAAAAAAAAA..	...		..AAA	(80A1)
NCHARS	NTEXT				(10I8)

NCHARS 	is the number of characters to be read from the next data field
        using the format (80A1)
NTEXT	is the number of lines of descriptive text before this data

FFFFFFFFFF..	...		..FFF	(80A1)
NFLOAT	NTEXT				(10I8)

NFLOAT 	is the number of floating point numbers to be read from the
        next data field using format (5E16.8)
NTEXT	is the number of lines of descriptive text before the data


IIIIIIIIII..	...		..III	(80A1)
NINTGR	NTEXT				(10I8)

NINTGR 	is the number of integer numbers to be read from the next data
        field using the format (10I8)
NTEXT	is the number of line of descriptive text before the data

JJJJJJJJJJ..	...		..JJJ	(80A1)
NINTGR	NTEXT				(8I10)

NINTGR 	is the number of integer numbers to be read from the next data
        field using the format (8I10), for use where the data are
        likely overwrite white space if written in I8 format.
NTEXT	is the number of line of descriptive text before the data

VVVVVVVVVV..	...		..VVV	(80A1)

Text data following are in a variable length format, and no further
standard fields are expected.
Examples
The sequence of key strings and data for typical instruments may be described in
an abbreviated form where each
capital letter, R,A,S,F,I,J,V denotes the initial key string, the small letter
the length information, and t and d denote
descriptive text and data strings respectively, all of fixed 80 characters total
string length. The data strings v are of
variable length (usually less than 256 characters, and most often less than 132
characters).
One spectrum/run

        RrtAatddFftttdddSsIittdddddddddddddddddddd

The program /usr/ill/bin/anadat can be used to analyse a file,
e.g. for D20 with one frame per run:

% /usr/ill/bin/anadat 050750
 anadat - version 3.1  June 2001  (R.E. Ghosh)


 Scanning file :050750
  Run  Format............ from  record      1
 50750 80A 480A  30F  25F  30F  15F  55F  20F +  1 x (  5F  1600J )
 End of file after    310  records


Several subspectra/run

        RrtAatddFftttdddSsIitdddddddSsIitdddddSsIitddddd..

A second example of D20 with 31 frames in a single file:
% /usr/ill/bin/anadat 044450
 anadat - version 3.1  June 2001  (R.E. Ghosh)


 Scanning file :044450
  Run  Format............ from  record      1
 44450 80A 480A  30F  25F  30F  15F  55F  20F + 31 x ( 30F  1600J )
 End of file after   6692  records

Variable format

        RrtAatddVvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

Apart from the run number the data are identified by the name of the instrument,
the date and time of the initial
recording, and a short experiment name. This information appears as a text field
immediately after the run number
key.
The text field thus follows an AAA key; at present (September 1994) the 80
characters are used as follows:
INSTexpt.-nameDD-MMM-YY.hh:mm:ss---48 blank ---- (80 total) where:
INST instrument (4 characters) expt.-name experiment name (10 characters)
DD-MMM-YY date of recording
(9 characters,one space) hh:mm:ss time of recording (8 characters) Example of a
data file from D11 Small-angle
scattering spectrometer
In addition to the standard header containing the instrument name etc., the
following 5 data fields are present:
        156I, 512A, 128F, 256I, 4096I

The formatted structure is:

RrAadIitddddddddddddddddAatdddddddFfttttttttddddddddddddddddddddddddddIitdd etc.



 *WIKI*/

#include "MantidMDAlgorithms/LoadILLAsciiHelper.h"
#include "MantidMDAlgorithms/DllConfig.h"

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <string>
#include <map>
#include <stdlib.h>
#include <limits>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/predicate.hpp>

namespace Mantid {
namespace MDAlgorithms {

/*
 * Constructor
 * @param filepath :: The filepath for the raw file.
 */
ILLParser::ILLParser(const std::string &filepath) {
  fin.open(filepath.c_str());
  if (!fin)
    throw(std::runtime_error("File does not appear to be valid: " + filepath));
}

ILLParser::~ILLParser() {
  if (fin) {
    fin.close();
  }
}

/*
 * Main function that parses the files a fills in the lists (see private
 * attributes of this class)
 */
void ILLParser::parse() {
  std::string line;
  while (std::getline(fin, line)) {
    if (line.find("RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR"
                  "RRRRRRRRRRRRRRRRRRRR") != std::string::npos) {
      parseFieldR();
    } else if (line.find("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                         "AAAAAAAAAAAAAAAAAAAAAAAAAAA") != std::string::npos) {
      parseFieldA();
    } else if (line.find("IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII"
                         "IIIIIIIIIIIIIIIIIIIIIIIIIII") != std::string::npos) {
      parseFieldNumeric(header, intWith);
    } else if (line.find("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
                         "FFFFFFFFFFFFFFFFFFFFFFFFFFF") != std::string::npos) {
      parseFieldNumeric(header, floatWith);
    } else if (line.find("SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS"
                         "SSSSSSSSSSSSSSSSSSSSSSSSSSS") != std::string::npos) {
      startParseSpectra();
    }
  }
}

/**
 * Reads the instrument name from the file
 * TODO:
 * This must be done before parsing the file, otherwise doesn't work:
 * fin.seekg(0, std::ios::beg) ; fin.clear();
 * If the file was already parsed, those functions don't appear to put
 * the get stream at the beginning. Needs to be fix.
 */
std::string ILLParser::getInstrumentName() {

  std::streamoff length = fin.tellg();
  if (length != std::ios::beg) {
    // TODO: this doesn't seem to work.
    fin.seekg(0, std::ios::beg);
    fin.clear();
    // throw std::runtime_error("Must be called before reading the file!");
  }

  std::string line, instrumentKeyword("Inst"), instrumentName("");
  int lineRead = 0, maxLineRead = 20; // never read more than

  while (std::getline(fin, line) && lineRead < maxLineRead) {

    if (boost::starts_with(line, instrumentKeyword)) {
      std::getline(fin, line);
      instrumentName = line.substr(0, instrumentKeyword.size());
      boost::algorithm::erase_all(instrumentName, " ");
    }
    lineRead += 1;
  }

  // Point to the beginning again!
  fin.seekg(0, std::ios::beg);
  fin.clear();
  return instrumentName;
}

/**
 * Parse fields of type R and keep them in the header
 */
void ILLParser::parseFieldR() {
  std::string line;
  std::getline(fin, line);
  std::vector<std::string> parsedLineFields =
      splitLineInFixedWithFields(line, intWith);
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
  std::vector<std::string> parsedLineFields =
      splitLineInFixedWithFields(line, intWith);
  // I'm not using this for now
  // int numberOfCharsToRead = evaluate<int>(parsedLineFields[0]);
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
  sscanf(line.c_str(), "%8d %8d", &nNumericFields, &nTextLines);

  std::vector<std::string> keys(nNumericFields);
  std::vector<std::string> values(nNumericFields);
  size_t index = 0;
  for (int i = 0; i < nTextLines; i++) {
    std::getline(fin, line);
    std::vector<std::string> s = splitLineInFixedWithFields(line, fieldWith);
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
    std::vector<std::string> s = splitLineInFixedWithFields(line, fieldWith);
    pos += s.size();
    for (auto it = s.begin(); it != s.end(); ++it) {
      values[index] = *it;
      index += 1;
    }
  }
  // keep the pairs key=value in the header
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
 * Parses the field I in the spectrum block
 * @param fieldWith :: will be either intWith or floatWith
 */
std::vector<int> ILLParser::parseFieldISpec(int fieldWith) {
  std::string line;
  std::getline(fin, line);
  int nSpectra;
  sscanf(line.c_str(), "%8d", &nSpectra);
  std::vector<int> spectrumValues(nSpectra);

  int nSpectraRead = 0, index = 0;
  while (nSpectraRead < nSpectra) {
    std::getline(fin, line);
    std::vector<std::string> s = splitLineInFixedWithFields(line, fieldWith);
    nSpectraRead += static_cast<int>(s.size());
    for (auto it = s.begin(); it != s.end(); ++it) {
      // sscanf is much faster than lexical_cast / erase_spaces
      sscanf(it->c_str(), "%8d", &spectrumValues[index]);
      index += 1;
    }
  }
  return spectrumValues;
}

/**
 * Shows contents of the headers
 * Just for debug purposes.
 */
void ILLParser::showHeader() {
  std::cout << "* Global header" << '\n';
  for (auto it = header.begin(); it != header.end(); ++it)
    std::cout << it->first << " => " << it->second << '\n';

  std::cout << "* Spectrum header" << '\n';
  int i = 0;
  std::vector<std::map<std::string, std::string>>::const_iterator s;
  for (s = spectraHeaders.begin(); s != spectraHeaders.end(); ++s) {
    std::cout << "** Spectrum i : " << i << '\n';
    std::map<std::string, std::string>::const_iterator it;
    for (it = s->begin(); it != s->end(); ++it)
      std::cout << it->first << " => " << it->second << ',';
    std::cout << std::endl;
    i++;
  }

  std::cout << "* Spectrum list" << '\n';
  std::vector<std::vector<int>>::const_iterator l;
  for (l = spectraList.begin(); l != spectraList.end(); ++l) {
    std::cout << "From " << (*l)[0] << " to " << (*l)[l->size() - 1] << " => "
              << l->size() << '\n';
  }
}

/**
 * Parses the spectrum blocks. Called after the header has been parsed.
 */
void ILLParser::startParseSpectra() {
  std::string line;
  std::getline(fin, line);
  while (std::getline(fin, line)) {
    if (line.find("IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII"
                  "IIIIIIIIIIIIIIIIIIII") != std::string::npos) {
      spectraList.push_back(parseFieldISpec());

    } else if (line.find("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
                         "FFFFFFFFFFFFFFFFFFFFFFFFFFF") != std::string::npos) {
      spectraHeaders.push_back(std::map<std::string, std::string>());
      parseFieldNumeric(spectraHeaders.back(), floatWith);
    } else if (line.find("SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS"
                         "SSSSSSSSSSSSSSSSSSSSSSSSSSS") != std::string::npos) {
      std::getline(fin, line);
    }
  }
}

/**
 * Splits a line in fixed length fields.
 */
std::vector<std::string>
ILLParser::splitLineInFixedWithFields(const std::string &s, int fieldWidth,
                                      int lineWitdh) {
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

/**
 * Evaluate the input string to a type T
 * @throws bad_lexical_cast if the input cannot be converted
 */
template <typename T> T ILLParser::evaluate(std::string field) {
  boost::algorithm::erase_all(field, " ");
  return boost::lexical_cast<T>(field);
}

/**
 * Gets a value from the header.
 */
template <typename T>
T ILLParser::getValueFromHeader(const std::string &field) {

  return getValue<T>(field, header);
}

/*
 * Get value type <T> from a map<strin,string>
 * @return  std::numeric_limits<T>::infinity() if no key was find in the map
 */
template <typename T>
T ILLParser::getValue(const std::string &field,
                      const std::map<std::string, std::string> &thisHeader) {

  T ret = std::numeric_limits<T>::infinity();

  for (auto it = thisHeader.begin(); it != thisHeader.end(); ++it) {
    // std::cout << it->first << "=>" << it->second << '\n';
    std::size_t pos = it->first.find(field);
    if (pos != std::string::npos) {
      // std::cout << "Found field: " << field << "=>" << it->second << '\n';
      ret = evaluate<T>(it->second);
    }
  }

  return ret;
}

} // namespace MDAlgorithms
} // namespace Mantid

// Concrete template instantiation
// The other libraries can't see the template definition so it needs to be
// instantiated in API
template double
Mantid::MDAlgorithms::ILLParser::getValueFromHeader(const std::string &);
template double Mantid::MDAlgorithms::ILLParser::getValue(
    const std::string &, const std::map<std::string, std::string> &);
