// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidKernel/FileDescriptor.h"

#include <list>

namespace Mantid {
namespace DataHandling {
/**
Loads a workspace from an ascii file. Spectra must be stored in columns.

Properties:
<ul>
<li>Filename  - the name of the file to read from.</li>
<li>Workspace - the workspace name that will be created and hold the loaded
data.</li>
<li>Separator - the column separation character: comma
(default),tab,space,colon,semi-colon.</li>
<li>Unit      - the unit to assign to the X axis (default: Energy).</li>
</ul>

@author Roman Tolchenov, Tessella plc
@date 3/07/09
*/
class DLLExport LoadAscii : public API::IFileLoader<Kernel::FileDescriptor>, public API::DeprecatedAlgorithm {
public:
  /// Default constructor
  LoadAscii();
  /// The name of the algorithm
  const std::string name() const override { return "LoadAscii"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads data from a text file and stores it in a 2D workspace "
           "(Workspace2D class).";
  }

  /// The version number
  int version() const override { return 1; }
  /// The category
  const std::string category() const override { return "DataHandling\\Text"; }
  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const override;

protected:
  /// Process the header information within the file.
  virtual void processHeader(std::ifstream &file) const;
  /// Read the data from the file
  virtual API::Workspace_sptr readData(std::ifstream &file) const;

  /// Peek at a line without extracting it from the stream
  void peekLine(std::ifstream &is, std::string &str) const;
  /// Return true if the line is to be skipped
  bool skipLine(const std::string &line) const;
  /// Split the data into columns.
  int splitIntoColumns(std::list<std::string> &columns, const std::string &str) const;
  /// Fill the given vector with the data values
  void fillInputValues(std::vector<double> &values, const std::list<std::string> &columns) const;

  /// The column separator
  std::string m_columnSep;

private:
  /// Declare properties
  void init() override;
  /// Execute the algorithm
  void exec() override;

  /// Map the separator options to their string equivalents
  std::map<std::string, std::string> m_separatorIndex;
};

} // namespace DataHandling
} // namespace Mantid
