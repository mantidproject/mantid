// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/MaskDetectorsIf.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/ListValidator.h"

#include <fstream>
#include <iomanip>

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(MaskDetectorsIf)

using namespace Kernel;

/** Initialisation method. Declares properties to be used in algorithm.
 */
void MaskDetectorsIf::init() {
  using namespace Mantid::Kernel;
  declareProperty(std::make_unique<API::WorkspaceProperty<>>("InputWorkspace", "",
                                                        Direction::Input),
                  "A 1D Workspace that contains values to select against");
  const std::vector<std::string> select_mode{"SelectIf", "DeselectIf"};
  declareProperty(
      "Mode", "SelectIf", boost::make_shared<StringListValidator>(select_mode),
      "Mode to select or deselect detectors based on comparison with values.");
  const std::vector<std::string> select_operator{
      "Equal", "NotEqual", "Greater", "GreaterEqual", "Less", "LessEqual"};
  declareProperty("Operator", "Equal",
                  boost::make_shared<StringListValidator>(select_operator),
                  "Unary operator to compare to given values.");
  declareProperty("Value", 0.0);
  declareProperty(
      std::make_unique<API::FileProperty>("InputCalFile", "",
                                     API::FileProperty::OptionalLoad, ".cal"),
      "The name of the CalFile with grouping data.");
  declareProperty(
      std::make_unique<API::FileProperty>("OutputCalFile", "",
                                     API::FileProperty::OptionalSave, ".cal"),
      "The name of the CalFile with grouping data.");
  declareProperty(std::make_unique<API::WorkspaceProperty<>>(
                      "OutputWorkspace", "", Direction::Output,
                      API::PropertyMode::Optional),
                  "The masked workspace.");
}

/**
 * Validates the algorithm's input properties.
 * @return A map from property name to reported issue
 */
std::map<std::string, std::string> MaskDetectorsIf::validateInputs() {
  std::map<std::string, std::string> issues;
  const auto noInputFile = isDefault("InputCalFile");
  const auto noOutputFile = isDefault("OutputCalFile");
  if (!noInputFile && noOutputFile) {
    issues["OutputCalFile"] = "Output file name is missing.";
  } else if (noInputFile && !noOutputFile) {
    issues["InputCalFile"] = "Input file name is missing.";
  }
  return issues;
}

/** Executes the algorithm
 */
void MaskDetectorsIf::exec() {
  retrieveProperties();

  if (isDefault("InputCalFile") && isDefault("OutputWorkspace")) {
    g_log.error() << "No InputCalFle or OutputWorkspace specified; the "
                     "algorithm will do nothing.";
    return;
  }
  const size_t nspec = m_inputW->getNumberHistograms();
  for (size_t i = 0; i < nspec; ++i) {
    // Get the list of udets contributing to this spectra
    const auto &dets = m_inputW->getSpectrum(i).getDetectorIDs();

    if (dets.empty())
      continue;
    else {
      const double val = m_inputW->y(i)[0];
      if (m_compar_f(val, m_value)) {
        for (const auto det : dets) {
          m_umap.emplace(det, m_select_on);
        }
      }
    }
    const double p = static_cast<double>(i) / static_cast<double>(nspec);
    progress(p, "Generating detector map");
  }

  if (!isDefault("InputCalFile")) {
    createNewCalFile();
  }
  if (!isDefault("OutputWorkspace")) {
    outputToWorkspace();
  }
}

/**
 * Create an output workspace masking/unmasking the selected/deselected spectra
 */
void MaskDetectorsIf::outputToWorkspace() {
  API::MatrixWorkspace_sptr outputW = getProperty("OutputWorkspace");
  if (outputW != m_inputW)
    outputW = m_inputW->clone();
  auto &detectorInfo = outputW->mutableDetectorInfo();
  for (const auto &selection : m_umap) {
    detectorInfo.setMasked(detectorInfo.indexOf(selection.first),
                           selection.second);
  }
  setProperty("OutputWorkspace", outputW);
}

/**
 * Get the input properties and store them in the object variables
 */
void MaskDetectorsIf::retrieveProperties() {
  m_inputW = getProperty("InputWorkspace");
  m_value = getProperty("Value");

  // Get the selction mode (select if or deselect if)
  std::string select_mode = getProperty("Mode");
  if (select_mode == "SelectIf")
    m_select_on = true;
  else
    m_select_on = false;

  // Select function object based on the type of comparison operator
  std::string select_operator = getProperty("Operator");

  if (select_operator == "LessEqual")
    m_compar_f = std::less_equal<double>();
  else if (select_operator == "Less")
    m_compar_f = std::less<double>();
  else if (select_operator == "GreaterEqual")
    m_compar_f = std::greater_equal<double>();
  else if (select_operator == "Greater")
    m_compar_f = std::greater<double>();
  else if (select_operator == "Equal")
    m_compar_f = std::equal_to<double>();
  else if (select_operator == "NotEqual")
    m_compar_f = std::not_equal_to<double>();
}

/**
 * Create a new cal file based on the old file
 * @throw Exception::FileError If a grouping file cannot be opened or read
 * successfully
 */
void MaskDetectorsIf::createNewCalFile() {
  const std::string oldfile = getProperty("InputCalFile");
  const std::string newfile = getProperty("OutputCalFile");
  progress(0.99, "Creating new cal file");
  std::ifstream oldf(oldfile.c_str());
  if (!oldf.is_open()) {
    g_log.error() << "Unable to open grouping file " << oldfile << '\n';
    throw Exception::FileError("Error reading .cal file", oldfile);
  }
  std::ofstream newf(newfile.c_str());
  if (!newf.is_open()) {
    g_log.error() << "Unable to open grouping file " << newfile << '\n';
    throw Exception::FileError("Error reading .cal file", newfile);
  }
  std::string str;
  while (getline(oldf, str)) {
    // Comment or empty lines get copied into the new cal file
    if (str.empty() || str[0] == '#') {
      newf << str << '\n';
      continue;
    }
    std::istringstream istr(str);
    int n, udet, sel, group;
    double offset;
    istr >> n >> udet >> offset >> sel >> group;
    const auto it = m_umap.find(udet);
    bool selection;

    if (it == m_umap.end())
      selection = sel != 0;
    else
      selection = (*it).second;

    newf << std::fixed << std::setw(9) << n << std::fixed << std::setw(15)
         << udet << std::fixed << std::setprecision(7) << std::setw(15)
         << offset << std::fixed << std::setw(8) << selection << std::fixed
         << std::setw(8) << group << '\n';
  }
  oldf.close();
  newf.close();
}
} // namespace Algorithms
} // namespace Mantid
