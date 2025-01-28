// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveCSV.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/Workspace2D.h"

#include <fstream> // used to get ofstream
#include <iomanip>

/* @class SaveCSV

 @author Anders J. Markvardsen, ISIS, RAL
 @date 15/10/2007
 */

namespace Mantid::DataHandling {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(SaveCSV)

using namespace Kernel;
using namespace API;
using API::MatrixWorkspace;
using API::MatrixWorkspace_sptr;
using API::WorkspaceProperty;
using DataObjects::Workspace2D;
using DataObjects::Workspace2D_sptr;

/// Empty default constructor
SaveCSV::SaveCSV() = default;

/** Initialisation method. Does nothing at present.
 *
 */
void SaveCSV::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "The filename of the output CSV file");
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Save, ".csv"),
                  "The name of the workspace containing the data you want to save to\n"
                  "a CSV file");
  declareProperty("Separator", ",",
                  "The separator that will go between the numbers on a line in the\n"
                  "output file (default ',')");
  getPointerToProperty("Separator")->setAutoTrim(false);
  declareProperty("LineSeparator", "\n",
                  "The string to place at the end of lines (default new line\n"
                  "character)");
  getPointerToProperty("LineSeparator")->setAutoTrim(false);
  declareProperty(std::make_unique<PropertyWithValue<bool>>("SaveXerrors", false, Direction::Input),
                  "This option saves out the x errors if any are present. If you have x "
                  "errors\n"
                  "in your workspace and you do not select this option, then the x errors\n"
                  "are not saved to the file.");
}

/** Executes the algorithm. Retrieve the Filename, separator and Lineseparator
 *  properties and save workspace to Filename.
 *
 *  @throw NotImplementedError Thrown if workspace to save is not a 1D or 2D
 *workspace
 *  @throw Mantid::Kernel::Exception::FileError  Thrown if errors with file
 *opening and existence
 */
void SaveCSV::exec() {
  // Gets the name of the file to save the workspace to; and the
  // separator and Lineseparator properties if they are provided by the user.

  // Retrieve the filename from the properties
  m_filename = getPropertyValue("Filename");

  // Get the values of the optional parameters
  m_separator = getPropertyValue("Separator");
  m_lineSeparator = getPropertyValue("LineSeparator");
  g_log.debug() << "Parameters: Filename='" << m_filename << "' "
                << "Seperator='" << m_separator << "' "
                << "LineSeparator='" << m_lineSeparator << "' \n";

  // prepare to save to file

  std::ofstream outCSV_File(m_filename.c_str());

  if (!outCSV_File) {
    g_log.error("Failed to open file:" + m_filename);
    throw Exception::FileError("Failed to open file:", m_filename);
  }

  // Get the input workspace
  const MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");

  // get workspace ID string. to check the data type

  const std::string workspaceID = inputWorkspace->id();
  // seperating out code depending on the workspace ID

  if (workspaceID.find("Workspace2D") != std::string::npos) {
    const Workspace2D_sptr localworkspace = std::dynamic_pointer_cast<Workspace2D>(inputWorkspace);

    // Get info from 2D workspace
    const size_t numberOfHist = localworkspace->getNumberHistograms();

    // Add first x-axis line to output file
    {
      const auto &xValue = localworkspace->x(0);

      outCSV_File << "A";

      for (double j : xValue) {
        outCSV_File << std::setw(15) << j << m_separator;
      }

      outCSV_File << m_lineSeparator;
      progress(0.2);
    }
    Progress p(this, 0.2, 1.0, 2 * numberOfHist);
    for (size_t i = 0; i < numberOfHist; i++) {
      // check if x-axis has changed. If yes print out new x-axis line

      if (i > 0) {
        auto &xValue = localworkspace->x(i);
        auto &xValuePrevious = localworkspace->x(i - 1);

        if (xValue.rawData() != xValuePrevious.rawData()) {
          outCSV_File << "A";

          for (double j : xValue) {
            outCSV_File << std::setw(15) << j << m_separator;
          }

          outCSV_File << m_lineSeparator;
        }
      }

      // add y-axis line for histogram (detector) i

      const auto &yValue = localworkspace->y(i);

      outCSV_File << i;

      for (double j : yValue) {
        outCSV_File << std::setw(15) << j << m_separator;
      }

      outCSV_File << m_lineSeparator;
      p.report();
    }
    // print out errors

    outCSV_File << "\nERRORS\n";

    for (size_t i = 0; i < numberOfHist; i++) {
      const auto &eValue = localworkspace->e(i);

      outCSV_File << i;

      for (double j : eValue) {
        outCSV_File << std::setw(15) << j << m_separator;
      }
      outCSV_File << m_lineSeparator;
      p.report();
    }

    // Potentially save the x errors
    if (getProperty("SaveXerrors")) {
      saveXerrors(outCSV_File, localworkspace, numberOfHist);
    }

  } else {
    outCSV_File.close(); // and should probably delete file from disk as well
    throw Exception::NotImplementedError("SaveCSV currently only works for 2D workspaces.");
  }
  outCSV_File.close();
}

void SaveCSV::saveXerrors(std::ofstream &stream, const Mantid::DataObjects::Workspace2D_sptr &workspace,
                          const size_t numberOfHist) {
  // If there isn't a dx values present in the first entry then return
  if (!workspace->hasDx(0)) {
    return;
  }
  Progress p(this, 0.0, 1.0, numberOfHist);
  stream << "\nXERRORS\n";
  for (size_t i = 0; i < numberOfHist; i++) {
    stream << i;

    for (double j : workspace->dx(i)) {
      stream << std::setw(15) << j << m_separator;
    }
    stream << m_lineSeparator;
    p.report("Saving x errors...");
  }
}

} // namespace Mantid::DataHandling
