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
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"

namespace Mantid {
namespace DataHandling {
/** @class SaveTBL SaveTBL.h DataHandling/SaveTBL.h

Saves a table workspace to a reflectometry tbl format ascii file.
Rows are 17 cells long and this save algorithm will throw if the workspace has
stitch groups of longer than 3 runs.
*/
class DLLExport SaveTBL : public API::Algorithm {
public:
  /// Default constructor
  SaveTBL();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SaveTBL"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Saves a table workspace to a reflectometry tbl format ascii file.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"LoadTBL"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Text"; }

private:
  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method
  void exec() override;
  /// Writes a value to the file
  template <class T> void writeVal(const T &val, std::ofstream &file, bool endsep = true, bool endline = false);
  void writeColumnNames(std::ofstream &file, std::vector<std::string> const &columnHeadings);
  /// the separator
  const char m_sep;
  // populates the map and vector containing grouping information
  void findGroups(const API::ITableWorkspace_sptr &ws);
  /// Map the separator options to their string equivalents
  std::map<int, std::vector<size_t>> m_stichgroups;
  std::vector<size_t> m_nogroup;
};
} // namespace DataHandling
} // namespace Mantid
