
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveDiffFittingAscii.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "Poco/File.h"
#include <boost/tokenizer.hpp>
#include <fstream>

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace Mantid::API;

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveDiffFittingAscii)

/// Empty constructor
SaveDiffFittingAscii::SaveDiffFittingAscii()
    : Mantid::API::Algorithm(), m_sep(','), m_counter(0) {}

/// Initialisation method.
void SaveDiffFittingAscii::init() {

  declareProperty(make_unique<WorkspaceProperty<ITableWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "The name of the workspace containing the data you want to "
                  "save to a TBL file");

  // Declare required parameters, filename with ext {.his} and input
  // workspace
  const std::vector<std::string> exts{".txt", ".csv", ""};
  declareProperty(Kernel::make_unique<API::FileProperty>(
                      "Filename", "", API::FileProperty::Save, exts),
                  "The filename to use for the saved data");

  declareProperty(
      "RunNumber", "", boost::make_shared<MandatoryValidator<std::string>>(),
      "Run number list of the focused files, which is used to generate the "
      "parameters table workspace");

  declareProperty(
      "Bank", "", boost::make_shared<MandatoryValidator<std::string>>(),
      "Bank number list of the focused files, which is used to generate "
      "the parameters table workspace");

  std::vector<std::string> formats;

  formats.push_back("AppendToExistingFile");
  formats.push_back("OverwriteFile");
  declareProperty("OutMode", "AppendToExistingFile",
                  boost::make_shared<Kernel::StringListValidator>(formats),
                  "Over write the file or append data to existing file");
}

/**
*   Executes the algorithm.
*/
void SaveDiffFittingAscii::exec() {

  // Retrieve the input workspace
  /// Workspace
  const ITableWorkspace_sptr tbl_ws = getProperty("InputWorkspace");
  if (!tbl_ws)
    throw std::runtime_error(
        "Please provide an input table workspace to be saved.");

  std::vector<API::ITableWorkspace_sptr> input_ws;
  input_ws.push_back(
      boost::dynamic_pointer_cast<DataObjects::TableWorkspace>(tbl_ws));

  processAll(input_ws);
}

bool SaveDiffFittingAscii::processGroups() {

  try {

    std::string name = getPropertyValue("InputWorkspace");

    WorkspaceGroup_sptr inputGroup =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(name);

    std::vector<API::ITableWorkspace_sptr> input_ws;
    for (int i = 0; i < inputGroup->getNumberOfEntries(); ++i) {
      input_ws.push_back(
          boost::dynamic_pointer_cast<ITableWorkspace>(inputGroup->getItem(i)));
    }

    // Store output workspace in AnalysisDataService
    if (!isChild())
      this->store();

    setExecuted(true);
    notificationCenter().postNotification(
        new FinishedNotification(this, this->isExecuted()));

    processAll(input_ws);
  } catch (...) {
    g_log.error()
        << "Error while processing groups on SaveDiffFittingAscii algorithm. "
        << '\n';
  }

  return true;
}

void SaveDiffFittingAscii::processAll(
    std::vector<API::ITableWorkspace_sptr> input_ws) {

  const std::string filename = getProperty("Filename");
  const std::string outMode = getProperty("OutMode");
  std::string runNumList = getProperty("RunNumber");
  std::string bankList = getProperty("Bank");

  Poco::File pFile = (filename);
  bool exist = pFile.exists();

  bool appendToFile = false;
  if (outMode == "AppendToExistingFile")
    appendToFile = true;

  // Initialize the file stream
  std::ofstream file(filename.c_str(),
                     (appendToFile ? std::ios::app : std::ios::out));

  if (!file) {
    throw Exception::FileError("Unable to create file: ", filename);
  }

  if (exist && !appendToFile) {
    g_log.warning() << "File " << filename << " exists and will be overwritten."
                    << "\n";
  }

  if (exist && appendToFile) {
    file << "\n";
  }

  // reset counter
  m_counter = 0;

  std::vector<std::string> splitRunNum = splitList(runNumList);
  std::vector<std::string> splitBank = splitList(bankList);

  // Create a progress reporting object
  Progress progress(this, 0, 1, input_ws.size());

  size_t breaker = input_ws.size();
  if (outMode == "AppendToExistingFile" && input_ws.size() == 1)
    breaker = 1;

  for (size_t i = 0; i < breaker; ++i) {

    std::string runNum = splitRunNum[m_counter];
    std::string bank = splitBank[m_counter];
    writeInfo(runNum, bank, file);

    // write header
    std::vector<std::string> columnHeadings = input_ws[i]->getColumnNames();
    writeHeader(columnHeadings, file);

    // write out the data form the table workspace
    size_t columnSize = columnHeadings.size();
    writeData(input_ws[i], file, columnSize);

    if (input_ws.size() > 1 && (i + 1) != input_ws.size()) {
      file << '\n';
    }
  }
  progress.report();
}

std::vector<std::string> SaveDiffFittingAscii::splitList(std::string strList) {
  std::vector<std::string> splitList;
  strList.erase(std::remove(strList.begin(), strList.end(), ' '),
                strList.end());
  boost::split(splitList, strList, boost::is_any_of(","));

  return splitList;
}

void SaveDiffFittingAscii::writeInfo(const std::string &runNumber,
                                     const std::string &bank,
                                     std::ofstream &file) {

  file << "run number: " << runNumber << '\n';
  file << "bank: " << bank << '\n';
  m_counter++;
}

void SaveDiffFittingAscii::writeHeader(std::vector<std::string> &columnHeadings,
                                       std::ofstream &file) {
  for (const auto &heading : columnHeadings) {
    // Chi being the last header in the table workspace
    if (heading == "Chi") {
      writeVal(heading, file, true);
    } else {
      writeVal(heading, file, false);
    }
  }
}

void SaveDiffFittingAscii::writeData(API::ITableWorkspace_sptr workspace,
                                     std::ofstream &file, size_t columnSize) {

  for (size_t rowIndex = 0; rowIndex < workspace->rowCount(); ++rowIndex) {
    TableRow row = workspace->getRow(rowIndex);
    for (size_t columnIndex = 0; columnIndex < columnSize; columnIndex++) {

      const auto row_str =
          boost::lexical_cast<std::string>(row.Double(columnIndex));
      g_log.debug() << row_str << std::endl;

      if (columnIndex == columnSize - 1)
        writeVal(row_str, file, true);
      else
        writeVal(row_str, file, false);
    }
  }
}

void SaveDiffFittingAscii::writeVal(const std::string &val, std::ofstream &file,
                                    bool endline) {
  std::string valStr = boost::lexical_cast<std::string>(val);

  // checking if it needs to be surrounded in
  // quotes due to a comma being included
  size_t comPos = valStr.find(',');
  if (comPos != std::string::npos) {
    file << '"' << val << '"';
  } else {
    file << boost::lexical_cast<std::string>(val);
  }

  if (endline) {
    file << '\n';
  } else {
    file << m_sep;
  }
}

} // namespace DataHandling
} // namespace Mantid
