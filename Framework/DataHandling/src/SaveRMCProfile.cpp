// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveRMCProfile.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/Unit.h"

#include <fstream>

namespace Mantid {
namespace DataHandling {

using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveRMCProfile)

/// Algorithm's name for identification. @see Algorithm::name
const std::string SaveRMCProfile::name() const { return "SaveRMCProfile"; }

/// Algorithm's version for identification. @see Algorithm::version
int SaveRMCProfile::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SaveRMCProfile::category() const {
  return "DataHandling\\Text";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string SaveRMCProfile::summary() const {
  return "Save files readable by RMCProfile";
}

/** Initialize the algorithm's properties.
 */
void SaveRMCProfile::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                                        Direction::Input),
                  "An input workspace to be saved.");

  declareProperty("InputType", "",
                  "To identify what input function is being used.");

  declareProperty("Title", "", "The title line for the output file.");

  declareProperty(std::make_unique<API::FileProperty>(
                      "Filename", "", API::FileProperty::Save, ".fq"),
                  "The filename to use for the saved data");
}

/// @copydoc Algorithm::validateInputs
std::map<std::string, std::string> SaveRMCProfile::validateInputs() {
  std::map<std::string, std::string> result;

  // check for null pointers - this is to protect against workspace groups
  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  if (!inputWS) {
    result["InputWorkspace"] = "Workspace not found";
  }

  const auto nHist = static_cast<int>(inputWS->getNumberHistograms());
  if (nHist != 1) {
    result["InputWorkspace"] = "Workspace must contain only one spectrum";
  }

  return result;
}

/** Execute the algorithm.
 */
void SaveRMCProfile::exec() {
  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  const std::string filename = getProperty("Filename");

  // --------- open the file
  std::ofstream out;
  out.open(filename.c_str(), std::ios_base::out);

  // --------- write the header in the style of required metadata
  writeMetaData(out, inputWS);

  // --------- write the data
  writeWSData(out, inputWS);

  // --------- close the file
  out.close();
}

void SaveRMCProfile::writeMetaData(std::ofstream &out,
                                   API::MatrixWorkspace_const_sptr inputWS) {
  const auto &y = inputWS->y(0);
  const std::string title = getProperty("Title");
  const std::string inputType = getProperty("InputType");
  out << y.size() << std::endl;
  out << "rmc " << inputType << " #  " << title << std::endl;
  std::cout << y.size() << std::endl;
  std::cout << "rmc " << inputType << " #  " << title << std::endl;
}

void SaveRMCProfile::writeWSData(std::ofstream &out,
                                 API::MatrixWorkspace_const_sptr inputWS) {
  const auto &x = inputWS->x(0);
  const auto &y = inputWS->y(0);
  const size_t length = x.size();
  if (x.size() == y.size()) {
    for (size_t i = 0; i < length; ++i) {
      out << "  " << x[i] << "  " << y[i] << "\n";
    }
  } else {
    for (size_t i = 0; i < length - 1; ++i) {
      out << "  " << (x[i] + x[i + 1]) / 2.0 << "  " << y[i] << "\n";
    }
  }
}

} // namespace DataHandling
} // namespace Mantid
