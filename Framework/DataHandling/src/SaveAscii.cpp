// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveAscii.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VisibleWhenProperty.h"

#include <boost/tokenizer.hpp>
#include <fstream>
#include <set>

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveAscii)

using namespace Kernel;
using namespace API;

/// Empty constructor
SaveAscii::SaveAscii() : m_separatorIndex() {}

/// Initialisation method.
void SaveAscii::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                            Direction::Input),
      "The name of the workspace containing the data you want to save to a "
      "Ascii file.");
  const std::vector<std::string> asciiExts{".dat", ".txt", ".csv"};
  declareProperty(std::make_unique<FileProperty>("Filename", "",
                                                 FileProperty::Save, asciiExts),
                  "The filename of the output Ascii file.");

  auto mustBeNonNegative = boost::make_shared<BoundedValidator<int>>();
  mustBeNonNegative->setLower(0);
  declareProperty("WorkspaceIndexMin", 0, mustBeNonNegative,
                  "The starting workspace index.");
  declareProperty("WorkspaceIndexMax", EMPTY_INT(), mustBeNonNegative,
                  "The ending workspace index.");
  declareProperty(std::make_unique<ArrayProperty<int>>("SpectrumList"),
                  "List of workspace indices to save.");
  declareProperty("Precision", EMPTY_INT(), mustBeNonNegative,
                  "Precision of output double values.");
  declareProperty(
      "WriteXError", false,
      "If true, the error on X will be written as the fourth column.");

  declareProperty("CommentIndicator", "",
                  "Character(s) to put in front of comment lines.");

  // For the ListValidator
  std::string spacers[6][2] = {
      {"CSV", ","},   {"Tab", "\t"},      {"Space", " "},
      {"Colon", ":"}, {"SemiColon", ";"}, {"UserDefined", "UserDefined"}};
  std::vector<std::string> sepOptions;
  for (auto &spacer : spacers) {
    std::string option = spacer[0];
    m_separatorIndex.insert(
        std::pair<std::string, std::string>(option, spacer[1]));
    sepOptions.push_back(option);
  }

  declareProperty("Separator", "CSV",
                  boost::make_shared<StringListValidator>(sepOptions),
                  "Character(s) to put as separator between X, Y, E values.");

  declareProperty(
      std::make_unique<PropertyWithValue<std::string>>("CustomSeparator", "",
                                                       Direction::Input),
      "If present, will override any specified choice given to Separator.");
  getPointerToProperty("CustomSeparator")->setAutoTrim(false);

  setPropertySettings("CustomSeparator",
                      std::make_unique<VisibleWhenProperty>(
                          "Separator", IS_EQUAL_TO, "UserDefined"));

  declareProperty("ColumnHeader", true,
                  "If true, put column headers into file. ");

  declareProperty("ICEFormat", false,
                  "If true, special column headers for ICE in file. ");
}

/**
 *   Executes the algorithm.
 */
void SaveAscii::exec() {
  // Get the workspace
  MatrixWorkspace_const_sptr ws = getProperty("InputWorkspace");
  auto nSpectra = static_cast<int>(ws->getNumberHistograms());
  auto nBins = static_cast<int>(ws->blocksize());

  // Get the properties
  std::vector<int> spec_list = getProperty("SpectrumList");
  int spec_min = getProperty("WorkspaceIndexMin");
  int spec_max = getProperty("WorkspaceIndexMax");
  bool writeHeader = getProperty("ColumnHeader");

  // Check whether we need to write the fourth column
  bool write_dx = getProperty("WriteXError");
  std::string choice = getPropertyValue("Separator");
  std::string custom = getPropertyValue("CustomSeparator");
  std::string sep;
  // If the custom separator property is not empty, then we use that under any
  // circumstance.
  if (!custom.empty()) {
    sep = custom;
  }
  // Else if the separator drop down choice is not UserDefined then we use that.
  else if (choice != "UserDefined") {
    auto it = m_separatorIndex.find(choice);
    sep = it->second;
  }
  // If we still have nothing, then we are forced to use a default.
  if (sep.empty()) {
    g_log.notice() << "\"UserDefined\" has been selected, but no custom "
                      "separator has been entered.  Using default instead.";
    sep = " , ";
  }
  std::string comment = getPropertyValue("CommentIndicator");
  std::string errstr = "E";
  std::string errstr2;
  std::string comstr = " , ";
  bool ice = getProperty("ICEFormat");
  if (ice) {
    // overwrite properties so file can be read by ICE
    errstr = "Y";
    errstr2 = "_error";
    comstr = ", ";
    writeHeader = true;
    write_dx = false;
    comment = "#features:";
  }

  // Create an spectra index list for output
  std::set<int> idx;

  // Add spectra interval into the index list
  if (spec_max != EMPTY_INT() && spec_min != EMPTY_INT()) {
    if (spec_min >= nSpectra || spec_max >= nSpectra || spec_min > spec_max)
      throw std::invalid_argument("Inconsistent spectra interval");
    for (int spec = spec_min; spec <= spec_max; spec++)
      idx.insert(spec);
  }

  // Add spectra list into the index list
  if (!spec_list.empty()) {
    for (auto &spec : spec_list) {
      if (spec >= nSpectra)
        throw std::invalid_argument("Inconsistent spectra list");
      else
        idx.insert(spec);
    }
  }
  if (!idx.empty())
    nSpectra = static_cast<int>(idx.size());

  if (nBins == 0 || nSpectra == 0)
    throw std::runtime_error("Trying to save an empty workspace");

  std::string filename = getProperty("Filename");
  std::ofstream file(filename.c_str());

  if (!file) {
    g_log.error("Unable to create file: " + filename);
    throw Exception::FileError("Unable to create file: ", filename);
  }

  // Write the column captions
  if (writeHeader) {
    file << comment << "X";
    if (idx.empty())
      for (int spec = 0; spec < nSpectra; spec++) {
        file << comstr << "Y" << spec << comstr << errstr << spec << errstr2;
        if (write_dx)
          file << " , DX" << spec;
      }
    else
      for (auto spec : idx) {
        file << comstr << "Y" << spec << comstr << errstr << spec << errstr2;
        if (write_dx)
          file << " , DX" << spec;
      }
    file << '\n';
  }

  // Set the number precision
  int prec = getProperty("Precision");
  if (prec != EMPTY_INT())
    file.precision(prec);

  Progress progress(this, 0.0, 1.0, nBins);
  auto pointDeltas = ws->pointStandardDeviations(0);
  auto points = ws->points(0);
  for (int bin = 0; bin < nBins; bin++) {
    file << points[bin];

    if (idx.empty())
      for (int spec = 0; spec < nSpectra; spec++) {
        file << sep;
        file << ws->y(spec)[bin];
        file << sep;
        file << ws->e(spec)[bin];
      }
    else
      for (auto spec : idx) {
        file << sep;
        file << ws->y(spec)[bin];
        file << sep;
        file << ws->e(spec)[bin];
      }

    if (write_dx) {
      file << sep;
      file << pointDeltas[bin];
    }
    file << '\n';
    progress.report();
  }
}

} // namespace DataHandling
} // namespace Mantid
