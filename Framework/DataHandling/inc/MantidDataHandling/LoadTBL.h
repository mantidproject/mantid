// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/FileDescriptor.h"

namespace Mantid {
namespace DataHandling {
/**
Loads a table workspace from an ascii file in reflectometry tbl format. Rows
must be no longer than 17 cells.
*/
class MANTID_DATAHANDLING_DLL LoadTBL : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  /// Default constructor
  LoadTBL();
  /// The name of the algorithm
  const std::string name() const override { return "LoadTBL"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads data from a reflectometry table file and stores it in a "
           "table workspace (TableWorkspace class).";
  }

  /// The version number
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"SaveTBL"}; }
  /// The category
  const std::string category() const override { return "DataHandling\\Text"; }
  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const override;

private:
  /// Declare properties
  void init() override;
  /// Execute the algorithm
  void exec() override;
  /// Split into Column headings with respect to comma delimiters
  bool getColumnHeadings(std::string line, std::vector<std::string> &cols);
  /// Split into columns with respect to the comma delimiters
  size_t getCells(std::string line, std::vector<std::string> &cols, size_t expectedCommas, bool isOldTBL) const;
  /// count the number of commas in the line
  size_t countCommas(const std::string &line) const;
  /// find all pairs of quotes in the line
  size_t findQuotePairs(const std::string &line, std::vector<std::vector<size_t>> &quoteBounds) const;
  /// Parse more complex CSV, used when the data involves commas in the data and
  /// quoted values
  void csvParse(const std::string &line, std::vector<std::string> &cols,
                const std::vector<std::vector<size_t>> &quoteBounds, size_t expectedCommas) const;
};

} // namespace DataHandling
} // namespace Mantid
