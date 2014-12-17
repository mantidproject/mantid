//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadReflTBL.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidKernel/Strings.h"
#include "MantidAPI/TableRow.h"
#include <fstream>

#include <boost/tokenizer.hpp>
#include <Poco/StringTokenizer.h>
// String utilities
#include <boost/algorithm/string.hpp>

namespace Mantid {
namespace DataHandling {
DECLARE_FILELOADER_ALGORITHM(LoadReflTBL);

using namespace Kernel;
using namespace API;

/// Empty constructor
LoadReflTBL::LoadReflTBL() : m_expectedCommas(16) {}

/**
* Return the confidence with with this algorithm can load the file
* @param descriptor A descriptor for the file
* @returns An integer specifying the confidence level. 0 indicates it will not
* be used
*/
int LoadReflTBL::confidence(Kernel::FileDescriptor &descriptor) const {
  const std::string &filePath = descriptor.filename();
  const size_t filenameLength = filePath.size();

  // Avoid some known file types that have different loaders
  int confidence(0);
  if (filePath.compare(filenameLength - 12, 12, "_runinfo.xml") == 0 ||
      filePath.compare(filenameLength - 6, 6, ".peaks") == 0 ||
      filePath.compare(filenameLength - 10, 10, ".integrate") == 0) {
    confidence = 0;
  } else if (descriptor.isAscii()) {
    std::istream &stream = descriptor.data();
    std::string firstLine;
    Kernel::Strings::extractToEOL(stream, firstLine);
    std::vector<std::string> columns;
    try {
      if (getCells(firstLine, columns) == 17) // right ammount of columns
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
    } catch (std::length_error) {
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
size_t LoadReflTBL::countCommas(std::string line) const {
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
size_t LoadReflTBL::findQuotePairs(
    std::string line, std::vector<std::vector<size_t>> &quoteBounds) const {
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
        quotepair.push_back(quoteOne);
        quotepair.push_back(quoteTwo);
        quoteBounds.push_back(quotepair);
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
* @throws std::length_error if anything other than 17 columns (or 16
* cell-delimiting commas) is found
*/
void
LoadReflTBL::csvParse(std::string line, std::vector<std::string> &cols,
                      std::vector<std::vector<size_t>> &quoteBounds) const {
  size_t pairID = 0;
  size_t valsFound = 0;
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
          cols.push_back(line.substr(quoteBounds.at(pairID).at(0) + 1,
                                     quoteBounds.at(pairID).at(1) -
                                         (quoteBounds.at(pairID).at(0) + 1)));
          ++pairID;
          ++valsFound;
        }
      } else {
        if (firstCell) {
          cols.push_back(line.substr(0, pos));
          firstCell = false;
        } else {
          cols.push_back(line.substr(lastComma + 1, pos - (lastComma + 1)));
        }
        ++valsFound;
      }
      lastComma = pos;
    } else {
      if (lastComma + 1 < line.length()) {
        cols.push_back(line.substr(lastComma + 1));
      } else {
        cols.push_back("");
      }
    }
  }
  if (cols.size() != 17) {
    std::string message =
        "A line must contain 16 cell-delimiting commas. Found " +
        boost::lexical_cast<std::string>(cols.size() - 1) + ".";
    throw std::length_error(message);
  }
}

/**
* Return the confidence with with this algorithm can load the file
* @param line the line to parse
* @param cols The vector to parse into
* @returns An integer specifying how many columns were parsed into. This should
* always be 17.
* @throws std::length_error if anything other than 17 columns (or 16
* cell-delimiting commas) is found
*/
size_t LoadReflTBL::getCells(std::string line,
                             std::vector<std::string> &cols) const {
  // first check the number of commas in the line.
  size_t found = countCommas(line);
  if (found == m_expectedCommas) {
    // If there are 16 that simplifies things and i can get boost to do the hard
    // work
    boost::split(cols, line, boost::is_any_of(","), boost::token_compress_off);
  } else if (found < m_expectedCommas) {
    // less than 16 means the line isn't properly formatted. So Throw
    std::string message =
        "A line must contain 16 cell-delimiting commas. Found " +
        boost::lexical_cast<std::string>(found) + ".";
    throw std::length_error(message);
  } else {
    // More than 16 will need further checks as more is only ok when pairs of
    // quotes surround a comma, meaning it isn't a delimiter
    std::vector<std::vector<size_t>> quoteBounds;
    findQuotePairs(line, quoteBounds);
    // if we didn't find any quotes, then there are too many commas and we
    // definitely have too many delimiters
    if (quoteBounds.size() == 0) {
      std::string message =
          "A line must contain 16 cell-delimiting commas. Found " +
          boost::lexical_cast<std::string>(found) + ".";
      throw std::length_error(message);
    }
    // now go through and split it up manually. Throw if we find ourselves in a
    // positon where we'd add a 18th value to the vector
    csvParse(line, cols, quoteBounds);
  }
  return cols.size();
}
//--------------------------------------------------------------------------
// Private methods
//--------------------------------------------------------------------------
/// Initialisation method.
void LoadReflTBL::init() {
  std::vector<std::string> exts;
  exts.push_back(".tbl");

  declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
                  "The name of the table file to read, including its full or "
                  "relative path. The file extension must be .tbl");
  declareProperty(new WorkspaceProperty<ITableWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
                  "The name of the workspace that will be created.");
}

/**
*   Executes the algorithm.
*/
void LoadReflTBL::exec() {
  std::string filename = getProperty("Filename");
  std::ifstream file(filename.c_str());
  if (!file) {
    throw Exception::FileError("Unable to open file: ", filename);
  }
  std::string line = "";

  ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();

  auto colRuns = ws->addColumn("str", "Run(s)");
  auto colTheta = ws->addColumn("str", "ThetaIn");
  auto colTrans = ws->addColumn("str", "TransRun(s)");
  auto colQmin = ws->addColumn("str", "Qmin");
  auto colQmax = ws->addColumn("str", "Qmax");
  auto colDqq = ws->addColumn("str", "dq/q");
  auto colScale = ws->addColumn("double", "Scale");
  auto colStitch = ws->addColumn("int", "StitchGroup");
  auto colOptions = ws->addColumn("str", "Options");

  colRuns->setPlotType(0);
  colTheta->setPlotType(0);
  colTrans->setPlotType(0);
  colQmin->setPlotType(0);
  colQmax->setPlotType(0);
  colDqq->setPlotType(0);
  colScale->setPlotType(0);
  colStitch->setPlotType(0);
  colOptions->setPlotType(0);

  std::vector<std::string> columns;

  int stitchID = 1;
  while (Kernel::Strings::extractToEOL(file, line)) {
    // ignore the row if the line is blank
    if (line == "" || line == ",,,,,,,,,,,,,,,,") {
      continue;
    }
    getCells(line, columns);

    const std::string scaleStr = columns.at(16);
    double scale = 1.0;
    if (!scaleStr.empty())
      Mantid::Kernel::Strings::convert<double>(columns.at(16), scale);

    // check if the first run in the row has any data associated with it
    // 0 = runs, 1 = theta, 2 = trans, 3 = qmin, 4 = qmax
    if (columns[0] != "" || columns[1] != "" || columns[2] != "" ||
        columns[3] != "" || columns[4] != "") {
      TableRow row = ws->appendRow();
      for (int i = 0; i < 5; ++i) {
        row << columns.at(i);
      }
      row << columns.at(15);
      row << scale;
      row << stitchID;
    }

    // check if the second run in the row has any data associated with it
    // 5 = runs, 6 = theta, 7 = trans, 8 = qmin, 9 = qmax
    if (columns[5] != "" || columns[6] != "" || columns[7] != "" ||
        columns[8] != "" || columns[9] != "") {
      TableRow row = ws->appendRow();
      for (int i = 5; i < 10; ++i) {
        row << columns.at(i);
      }
      row << columns.at(15);
      row << scale;
      row << stitchID;
    }

    // check if the third run in the row has any data associated with it
    // 10 = runs, 11 = theta, 12 = trans, 13 = qmin, 14 = qmax
    if (columns[10] != "" || columns[11] != "" || columns[12] != "" ||
        columns[13] != "" || columns[14] != "") {
      TableRow row = ws->appendRow();
      for (int i = 10; i < 17; ++i) {
        if (i == 16)
          row << scale;
        else
          row << columns.at(i);
      }
      row << stitchID;
    }
    ++stitchID;
  }

  setProperty("OutputWorkspace", ws);
}

} // namespace DataHandling
} // namespace Mantid
