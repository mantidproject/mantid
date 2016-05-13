
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidDataHandling/SaveTBL.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/TableWorkspace.h"
#include <fstream>
#include "Poco/File.h"
#include <boost/tokenizer.hpp>

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveTBL)

using namespace Kernel;
using namespace API;

/// Empty constructor
SaveTBL::SaveTBL() : m_sep(','), m_stichgroups(), m_nogroup() {}

/// Initialisation method.
void SaveTBL::init() {
  declareProperty(
      make_unique<FileProperty>("Filename", "", FileProperty::Save, ".tbl"),
      "The filename of the output TBL file.");

  declareProperty(make_unique<WorkspaceProperty<ITableWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "The name of the workspace containing the data you want to "
                  "save to a TBL file.");
}

/**
* Finds the stitch groups that need to be on the same line
* @param ws : a pointer to a tableworkspace
*/
void SaveTBL::findGroups(ITableWorkspace_sptr ws) {
  size_t rowCount = ws->rowCount();
  for (size_t i = 0; i < rowCount; ++i) {
    TableRow row = ws->getRow(i);
    if (row.cell<int>(ws->columnCount() - 2) != 0) {
      // it was part of a group
      m_stichgroups[row.cell<int>(ws->columnCount() - 2)].push_back(i);
    } else {
      // it wasn't part of a group
      m_nogroup.push_back(i);
    }
  }
}

/**
*   Executes the algorithm.
*/
void SaveTBL::exec() {
  // Get the workspace
  ITableWorkspace_sptr ws = getProperty("InputWorkspace");
  if (!ws)
    throw std::runtime_error("Please provide an input workspace to be saved.");
  findGroups(ws);
  std::string filename = getProperty("Filename");
  std::ofstream file(filename.c_str());

  if (!file) {
    throw Exception::FileError("Unable to create file: ", filename);
  }
  std::vector<std::string> columnHeadings = ws->getColumnNames();
  for (auto &heading : columnHeadings) {
    if (heading == "Options")
      writeVal<std::string>(heading, file, false, true);
    else
      writeVal<std::string>(heading, file);
  }
  for (size_t rowIndex = 0; rowIndex < ws->rowCount(); ++rowIndex) {
    TableRow row = ws->getRow(rowIndex);
    for (size_t columnIndex = 0; columnIndex < columnHeadings.size();
         columnIndex++) {
      if (columnIndex == columnHeadings.size() - 2) {
        std::string groupHeading = columnHeadings[columnIndex];
        if (ws->getColumn(groupHeading)->type() != "int") {
          file.close();
          remove(filename.c_str());
          throw std::runtime_error(groupHeading +
                                   " Column must be of type \"int\"");
        } else
          writeVal<int>(row.Int(columnIndex), file);
      } else if (ws->getColumn(columnIndex)->type() != "str") {
        file.close();
        remove(filename.c_str());
        throw std::runtime_error(columnHeadings[columnIndex] +
                                 " columns must be of type \"str\"");
      } else {
        if (columnIndex == columnHeadings.size() - 1)
          writeVal<std::string>(row.String(columnIndex), file, false, true);
        else
          writeVal<std::string>(row.String(columnIndex), file);
      }

    } // col for-loop
  }   // row for-loop
  file.close();
}

/**
* Writes the given value to file, checking if it needs to be surrounded in
* quotes due to a comma being included
* @param val : the string to be written
* @param file : the ouput file stream
* @param endsep : boolean true to include a comma after the data
* @param endline : boolean true to put an EOL at the end of this data value
*/
template <class T>
void SaveTBL::writeVal(T &val, std::ofstream &file, bool endsep, bool endline) {
  std::string valStr = boost::lexical_cast<std::string>(val);
  size_t comPos = valStr.find(',');
  if (comPos != std::string::npos) {
    file << '"' << val << '"';
  } else {
    file << boost::lexical_cast<T>(val);
  }
  if (endsep) {
    file << m_sep;
  }
  if (endline) {
    file << std::endl;
  }
}
} // namespace DataHandling
} // namespace Mantid
