// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadTBL.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/Strings.h"
#include <fstream>

#include "MantidKernel/StringTokenizer.h"
#include <boost/tokenizer.hpp>
// String utilities
#include <boost/algorithm/string.hpp>

namespace Mantid::DataHandling {
DECLARE_FILELOADER_ALGORITHM(LoadTBL)

using namespace Kernel;
using namespace API;

/// Empty constructor
LoadTBL::LoadTBL() = default;

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadTBL::confidence(Kernel::FileDescriptor &descriptor) const {
  const std::string &filePath = descriptor.filename();
  const size_t filenameLength = filePath.size();

  // Avoid some known file types that have different loaders
  int confidence(0);
  if (filenameLength > 12            ? (filePath.compare(filenameLength - 12, 12, "_runinfo.xml") == 0)
      : false || filenameLength > 6  ? (filePath.compare(filenameLength - 6, 6, ".peaks") == 0)
      : false || filenameLength > 10 ? (filePath.compare(filenameLength - 10, 10, ".integrate") == 0)
                                     : false) {
    confidence = 0;
  } else if (descriptor.isAscii()) {
    std::istream &stream = descriptor.data();
    std::string firstLine;
    Kernel::Strings::extractToEOL(stream, firstLine);
    try {
      std::vector<std::string> columns;
      if (getCells(firstLine, columns, 16, true) == 17) // right ammount of columns
      {
        if (filePath.compare(filenameLength - 4, 4, ".tbl") == 0) {
          confidence = 40;
        } else {
          confidence = 20;
        }
      } else // incorrect amount of columns
      {
        confidence = 0;
      }
    } catch (const std::length_error &) {
      confidence = 0;
    }
  }
  return confidence;
}

/**
 * counte the commas in the line
 * @param line the line to count from
 * @returns a size_t of how many commas were in line
 */
size_t LoadTBL::countCommas(const std::string &line) const {
  size_t found = 0;
  size_t pos = line.find(',', 0);
  if (pos != std::string::npos) {
    ++found;
  }
  while (pos != std::string::npos) {
    pos = line.find(',', pos + 1);
    if (pos != std::string::npos) {
      ++found;
    }
  }
  return found;
}

/**
 * find pairs of qutoes and store them in a vector
 * @param line the line to count from
 * @param quoteBounds a vector<vector<size_t>> which will contain the locations
 * of pairs of quotes
 * @returns a size_t of how many pairs of quotes were in line
 */
size_t LoadTBL::findQuotePairs(const std::string &line, std::vector<std::vector<size_t>> &quoteBounds) const {
  size_t quoteOne = 0;
  size_t quoteTwo = 0;
  while (quoteOne != std::string::npos && quoteTwo != std::string::npos) {
    if (quoteTwo == 0) {
      quoteOne = line.find('"');
    } else {
      quoteOne = line.find('"', quoteTwo + 1);
    }
    if (quoteOne != std::string::npos) {
      quoteTwo = line.find('"', quoteOne + 1);
      if (quoteTwo != std::string::npos) {
        std::vector<size_t> quotepair;
        quotepair.emplace_back(quoteOne);
        quotepair.emplace_back(quoteTwo);
        quoteBounds.emplace_back(quotepair);
      }
    }
  }
  return quoteBounds.size();
}

/**
 * parse the CSV format if it's not a simple case of splitting 16 commas
 * @param line the line to parse
 * @param cols The vector to parse into
 * @param quoteBounds a vector<vector<size_t>> containing the locations of pairs
 * of quotes
 * @param expectedCommas The number of expected commas in the line
 * @throws std::length_error if anything other than 17 columns (or 16
 * cell-delimiting commas) is found
 */
void LoadTBL::csvParse(const std::string &line, std::vector<std::string> &cols,
                       std::vector<std::vector<size_t>> &quoteBounds, size_t expectedCommas) const {
  size_t pairID = 0;
  size_t lastComma = 0;
  size_t pos = 0;
  bool firstCheck = true;
  bool firstCell = true;
  cols.clear();
  while (pos != std::string::npos) {
    if (firstCheck) {
      pos = line.find(',');
      firstCheck = false;
      // lastpos = pos;
    } else {
      pos = line.find(',', pos + 1);
      // lastpos = pos;
    }
    if (pos != std::string::npos) {
      if (pairID < quoteBounds.size() && pos > quoteBounds.at(pairID).at(0)) {
        if (pos > quoteBounds.at(pairID).at(1)) {
          // use the quote indexes to get the substring
          cols.emplace_back(line.substr(quoteBounds.at(pairID).at(0) + 1,
                                        quoteBounds.at(pairID).at(1) - (quoteBounds.at(pairID).at(0) + 1)));
          ++pairID;
        }
      } else {
        if (firstCell) {
          cols.emplace_back(line.substr(0, pos));
          firstCell = false;
        } else {
          auto colVal = line.substr(lastComma + 1, pos - (lastComma + 1));
          cols.emplace_back(line.substr(lastComma + 1, pos - (lastComma + 1)));
        }
      }
      lastComma = pos;
    } else {
      if (lastComma + 1 < line.length()) {
        cols.emplace_back(line.substr(lastComma + 1));
      } else {
        cols.emplace_back("");
      }
    }
  }
  if (cols.size() != expectedCommas + 1) {
    std::string message = "A line must contain " + std::to_string(expectedCommas) + " cell-delimiting commas. Found " +
                          std::to_string(cols.size() - 1) + ".";
    throw std::length_error(message);
  }
}

/**
 * Return the confidence with with this algorithm can load the file
 * @param line the line to parse
 * @param cols The vector to parse into
 * @param expectedCommas The number of expected commas in the line
 * @param isOldTBL boolean to deal with new and old TBL formats.
 * @returns An integer specifying how many columns were parsed into.
 * @throws std::length_error if anything other than 17 columns (or 16
 * cell-delimiting commas) is found when loading an old Refl TBL. A
 * length_error will be thrown for new TBL formats if there are less column
 * headings than expected commas.
 */
size_t LoadTBL::getCells(std::string line, std::vector<std::string> &cols, size_t expectedCommas, bool isOldTBL) const {
  // first check the number of commas in the line.
  size_t found = countCommas(line);
  if (isOldTBL) {
    if (found == expectedCommas) {
      // If there are 16 that simplifies things and i can get boost to do the
      // hard
      // work
      boost::split(cols, line, boost::is_any_of(","), boost::token_compress_off);
    } else if (found < expectedCommas) {
      // less than 16 means the line isn't properly formatted. So Throw
      std::string message = "A line must contain " + std::to_string(expectedCommas) +
                            " cell-delimiting commas. Found " + std::to_string(found) + ".";
      throw std::length_error(message);
    } else {
      // More than 16 will need further checks as more is only ok when pairs of
      // quotes surround a comma, meaning it isn't a delimiter
      std::vector<std::vector<size_t>> quoteBounds;
      findQuotePairs(line, quoteBounds);
      // if we didn't find any quotes, then there are too many commas and we
      // definitely have too many delimiters
      if (quoteBounds.empty()) {
        std::string message = "A line must contain " + std::to_string(expectedCommas) +
                              " cell-delimiting commas. Found " + std::to_string(found) + ".";
        throw std::length_error(message);
      }
      // now go through and split it up manually. Throw if we find ourselves in
      // a
      // positon where we'd add a 18th value to the vector
      csvParse(line, cols, quoteBounds, expectedCommas);
    }
  } else {
    std::vector<std::vector<size_t>> quoteBounds;
    findQuotePairs(line, quoteBounds);
    csvParse(line, cols, quoteBounds, expectedCommas);
    if (cols.size() > expectedCommas) {
      for (size_t i = expectedCommas + 1; i < cols.size(); i++) {
        cols[expectedCommas].append(boost::lexical_cast<std::string>("," + cols[i]));
      }
    } else if (cols.size() < expectedCommas) {
      std::string message = "A line must contain " + std::to_string(expectedCommas) +
                            " cell-delimiting commas. Found " + std::to_string(found) + ".";
      throw std::length_error(message);
    }
  }
  return cols.size();
}
bool LoadTBL::getColumnHeadings(std::string line, std::vector<std::string> &cols) {
  boost::split(cols, line, boost::is_any_of(","), boost::token_compress_off);
  std::string firstEntry = cols[0];
  if (std::all_of(firstEntry.begin(), firstEntry.end(), ::isdigit)) {
    // TBL file contains column headings
    cols.clear();
    return true;
  } else {
    return false;
  }
}
//--------------------------------------------------------------------------
// Private methods
//--------------------------------------------------------------------------
/// Initialisation method.
void LoadTBL::init() {
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, ".tbl"),
                  "The name of the table file to read, including its full or "
                  "relative path. The file extension must be .tbl");
  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The name of the workspace that will be created.");
}

/**
 *   Executes the algorithm.
 */
void LoadTBL::exec() {
  std::string filename = getProperty("Filename");
  std::ifstream file(filename.c_str());
  if (!file) {
    throw Exception::FileError("Unable to open file: ", filename);
  }
  std::string line;

  ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();

  std::vector<std::string> columnHeadings;

  Kernel::Strings::extractToEOL(file, line);
  // We want to check if the first line contains an empty string or series of
  // ",,,,,"
  // to see if we are loading a TBL file that actually contains data or not.
  boost::split(columnHeadings, line, boost::is_any_of(","), boost::token_compress_off);
  for (auto entry = columnHeadings.begin(); entry != columnHeadings.end();) {
    if (entry->empty()) {
      // erase the empty values
      entry = columnHeadings.erase(entry);
    } else {
      // keep any non-empty values
      ++entry;
    }
  }
  if (columnHeadings.empty()) {
    // we have an empty string or series of ",,,,,"
    throw std::runtime_error("The file you are trying to load is Empty. \n "
                             "Please load a non-empty TBL file");
  } else {
    // set columns back to empty ready to populated with columnHeadings.
    columnHeadings.clear();
  }
  // this will tell us if we need to just fill in the cell values
  // or whether we will have to create the column headings as well.
  bool isOld = getColumnHeadings(line, columnHeadings);

  std::vector<std::string> rowVec;
  if (isOld) {
    /**THIS IS ESSENTIALLY THE OLD LoadReflTBL CODE**/
    // create the column headings
    ws->addColumn("str", "StitchGroup");
    ws->addColumn("str", "Run(s)");
    ws->addColumn("str", "ThetaIn");
    ws->addColumn("str", "TransRun(s)");
    ws->addColumn("str", "Qmin");
    ws->addColumn("str", "Qmax");
    ws->addColumn("str", "dq/q");
    ws->addColumn("str", "Scale");
    ws->addColumn("str", "Options");
    ws->addColumn("str", "HiddenOptions");

    for (size_t i = 0; i < ws->columnCount(); i++) {
      auto col = ws->getColumn(i);
      col->setPlotType(0);
    }

    // we are using the old ReflTBL format
    // where all of the entries are on one line
    // so we must reset the stream to reread the first line.
    std::ifstream fileReopened(filename.c_str());
    if (!fileReopened) {
      throw Exception::FileError("Unable to open file: ", filename);
    }
    std::string lineRevisited;
    int stitchID = 1;
    while (Kernel::Strings::extractToEOL(fileReopened, lineRevisited)) {
      if (lineRevisited.empty() || lineRevisited == ",,,,,,,,,,,,,,,,") {
        continue;
      }
      getCells(lineRevisited, rowVec, 16, isOld);
      const std::string scaleStr = rowVec.at(16);
      const std::string stitchStr = boost::lexical_cast<std::string>(stitchID);

      // check if the first run in the row has any data associated with it
      // 0 = runs, 1 = theta, 2 = trans, 3 = qmin, 4 = qmax
      if (!rowVec[0].empty() || !rowVec[1].empty() || !rowVec[2].empty() || !rowVec[3].empty() || !rowVec[4].empty()) {
        TableRow row = ws->appendRow();
        row << stitchStr;
        for (int i = 0; i < 5; ++i) {
          row << rowVec.at(i);
        }
        row << rowVec.at(15);
        row << scaleStr;
      }

      // check if the second run in the row has any data associated with it
      // 5 = runs, 6 = theta, 7 = trans, 8 = qmin, 9 = qmax
      if (!rowVec[5].empty() || !rowVec[6].empty() || !rowVec[7].empty() || !rowVec[8].empty() || !rowVec[9].empty()) {
        TableRow row = ws->appendRow();
        row << stitchStr;
        for (int i = 5; i < 10; ++i) {
          row << rowVec.at(i);
        }
        row << rowVec.at(15);
        row << scaleStr;
      }

      // check if the third run in the row has any data associated with it
      // 10 = runs, 11 = theta, 12 = trans, 13 = qmin, 14 = qmax
      if (!rowVec[10].empty() || !rowVec[11].empty() || !rowVec[12].empty() || !rowVec[13].empty() ||
          !rowVec[14].empty()) {
        TableRow row = ws->appendRow();
        row << stitchStr;
        for (int i = 10; i < 17; ++i) {
          if (i == 16)
            row << scaleStr;
          else
            row << rowVec.at(i);
        }
      }
      ++stitchID;
      setProperty("OutputWorkspace", ws);
    }

  } else {
    // we have a TBL format that contains column headings
    // on the first row. These are now entries in the columns vector
    if (!columnHeadings.empty()) {
      // now we need to add the custom column headings from
      // the columns vector to the TableWorkspace
      for (auto heading = columnHeadings.begin(); heading != columnHeadings.end();) {
        if (heading->empty()) {
          // there is no need to have empty column headings.
          heading = columnHeadings.erase(heading);
        } else {
          Mantid::API::Column_sptr col;
          col = ws->addColumn("str", *heading);
          col->setPlotType(0);
          heading++;
        }
      }
    }
    size_t expectedCommas = columnHeadings.size() - 1;
    while (Kernel::Strings::extractToEOL(file, line)) {
      if (line.empty() || line == ",,,,,,,,,,,,,,,,") {
        // skip over any empty lines
        continue;
      }
      getCells(line, rowVec, columnHeadings.size() - 1, isOld);
      // populate the columns with their values for this row.
      TableRow row = ws->appendRow();
      for (size_t i = 0; i < expectedCommas + 1; ++i) {
        row << rowVec.at(i);
      }
    }
    setProperty("OutputWorkspace", ws);
  }
}

} // namespace Mantid::DataHandling
