// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveDiffFittingAscii.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "Poco/File.h"
#include <boost/tokenizer.hpp>
#include <fstream>

namespace Mantid::DataHandling {

using namespace Kernel;
using namespace Mantid::API;

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveDiffFittingAscii)

/// Empty constructor
SaveDiffFittingAscii::SaveDiffFittingAscii() : Mantid::API::Algorithm(), m_sep(','), m_counter(0) {
  useAlgorithm("EnggSaveSinglePeakFitResultsToHDF5", 1);
}

/// Initialisation method.
void SaveDiffFittingAscii::init() {

  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>("InputWorkspace", "", Direction::Input),
                  "The name of the workspace containing the data you want to "
                  "save to a TBL file");

  // Declare required parameters, filename with ext {.his} and input
  // workspace
  const std::vector<std::string> exts{".txt", ".csv", ""};
  declareProperty(std::make_unique<API::FileProperty>("Filename", "", API::FileProperty::Save, exts),
                  "The filename to use for the saved data");

  declareProperty("RunNumber", "",
                  "Run number list of the focused files, which is used to generate the "
                  "parameters table workspace");

  declareProperty("Bank", "",
                  "Bank number list of the focused files, which is used to generate "
                  "the parameters table workspace");

  std::vector<std::string> formats;

  formats.emplace_back("AppendToExistingFile");
  formats.emplace_back("OverwriteFile");
  declareProperty("OutMode", "AppendToExistingFile", std::make_shared<Kernel::StringListValidator>(formats),
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
    throw std::runtime_error("Please provide an input table workspace to be saved.");

  std::vector<API::ITableWorkspace_sptr> input_ws;
  input_ws.emplace_back(std::dynamic_pointer_cast<DataObjects::TableWorkspace>(tbl_ws));

  processAll(input_ws);
}

bool SaveDiffFittingAscii::processGroups() {

  try {

    std::string wsName = getPropertyValue("InputWorkspace");

    WorkspaceGroup_sptr inputGroup = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(wsName);

    std::vector<API::ITableWorkspace_sptr> input_ws;
    input_ws.reserve(inputGroup->getNumberOfEntries());
    for (int i = 0; i < inputGroup->getNumberOfEntries(); ++i) {
      input_ws.emplace_back(std::dynamic_pointer_cast<ITableWorkspace>(inputGroup->getItem(i)));
    }

    processAll(input_ws);
  } catch (std::runtime_error &rexc) {
    g_log.error(std::string("Error while processing a group of workspaces. Details: ") + rexc.what() + '\n');
  }

  return true;
}

void SaveDiffFittingAscii::processAll(const std::vector<API::ITableWorkspace_sptr> &input_ws) {

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
  std::ofstream file(filename.c_str(), (appendToFile ? std::ios::app : std::ios::out));

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
  Progress progress(this, 0.0, 1.0, input_ws.size());

  size_t breaker = input_ws.size();
  if (outMode == "AppendToExistingFile" && input_ws.size() == 1)
    breaker = 1;

  for (size_t i = 0; i < breaker; ++i) {

    std::string runNum = splitRunNum[m_counter];
    std::string bank = splitBank[m_counter];

    if (!runNum.empty() || !bank.empty())
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
  strList.erase(std::remove(strList.begin(), strList.end(), ' '), strList.end());
  boost::split(splitList, strList, boost::is_any_of(","));

  return splitList;
}

void SaveDiffFittingAscii::writeInfo(const std::string &runNumber, const std::string &bank, std::ofstream &file) {

  file << "run number: " << runNumber << '\n';
  file << "bank: " << bank << '\n';
  m_counter++;
}

void SaveDiffFittingAscii::writeHeader(const std::vector<std::string> &columnHeadings, std::ofstream &file) {
  for (const auto &heading : columnHeadings) {
    // if last header in the table workspace, put eol
    if (&heading == &columnHeadings.back()) {
      writeVal(heading, file, true);
    } else {
      writeVal(heading, file, false);
    }
  }
}

void SaveDiffFittingAscii::writeData(const API::ITableWorkspace_sptr &workspace, std::ofstream &file,
                                     const size_t columnSize) {

  for (size_t rowIndex = 0; rowIndex < workspace->rowCount(); ++rowIndex) {
    TableRow row = workspace->getRow(rowIndex);
    for (size_t columnIndex = 0; columnIndex < columnSize; columnIndex++) {

      const auto row_str = boost::lexical_cast<std::string>(row.Double(columnIndex));
      g_log.debug() << row_str << std::endl;

      if (columnIndex == columnSize - 1)
        writeVal(row_str, file, true);
      else
        writeVal(row_str, file, false);
    }
  }
}

void SaveDiffFittingAscii::writeVal(const std::string &val, std::ofstream &file, const bool endline) {
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

std::map<std::string, std::string> SaveDiffFittingAscii::validateInputs() {
  std::map<std::string, std::string> errors;

  bool is_grp = true;

  // check for null pointers - this is to protect against workspace groups
  const std::string inputWS = getProperty("InputWorkspace");

  WorkspaceGroup_sptr inWks = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(inputWS);
  API::WorkspaceGroup_const_sptr inGrp = std::dynamic_pointer_cast<const API::WorkspaceGroup>(inWks);

  const ITableWorkspace_sptr tbl_ws = getProperty("InputWorkspace");
  if (tbl_ws) {
    is_grp = false;
  }

  if (!inGrp && !tbl_ws) {
    std::string message = "The current version of this algorithm only "
                          "supports input workspaces of type TableWorkspace and WorkspaceGroup";
    errors["InputWorkspace"] = message;
  }

  const std::string file = getProperty("Filename");
  if (file.empty()) {
    errors["Filename"] = "File name directory cannot be empty";
  }

  std::string runNumber = getPropertyValue("RunNumber");
  std::vector<std::string> splitRunNum = splitList(runNumber);

  std::string bankNumber = getPropertyValue("Bank");
  std::vector<std::string> splitBank = splitList(bankNumber);

  if (bankNumber.empty()) {
    if (!runNumber.empty())
      errors["Bank"] = "Please provide a valid bank list";
  } else if (runNumber.empty()) {
    errors["RunNumber"] = "Please provide a valid run number list";
  } else if (!is_grp) {
    if (splitRunNum.size() > 1) {
      errors["RunNumber"] = "One run number should be provided when a Table"
                            "workspace is selected";
    }
    if (splitBank.size() > 1) {
      errors["Bank"] = "One bank should be provided when a Table"
                       "Workspace is selected";
    }
  } else {
    if (splitRunNum.size() != inGrp->size()) {
      errors["RunNumber"] = "Run number list size should match the number of "
                            "TableWorkspaces in the GroupWorkspace selected";
    }
    if (splitBank.size() != inGrp->size()) {
      errors["Bank"] = "Bank list size should match the number of "
                       "TableWorkspaces in the GroupWorkspace selected";
    }
  }

  return errors;
}

} // namespace Mantid::DataHandling
