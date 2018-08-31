#ifndef MANTID_DATAHANDLING_SaveReflectometryAscii_H_
#define MANTID_DATAHANDLING_SaveReflectometryAscii_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace Mantid {
namespace DataHandling {
/**
Saves a file in MFT Ascii format from a 2D workspace.

Copyright &copy; 2007-18 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport SaveReflectometryAscii : public API::Algorithm {
public:
  /// Algorithm's name. @see Algorithm::name
  const std::string name() const override { return "SaveReflectometryAscii"; }
  /// Algorithm's version. @see Algorithm::version
  int version() const override { return 1; }
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string category() const override {
    return "DataHandling\\Text;ILL\\Reflectometry;Reflectometry";
  }
  /// Summary of algorithms purpose. @see Algorithm::summary
  const std::string summary() const override {
    return "Saves a 2D workspace to an ascii file";
  }
  /// Algorithm's with similar purpose. @see Algorithm::seeAlso
  const std::vector<std::string> seeAlso() const override {
    return {"SaveAscii"};
  }
  /// Cross-check properties with each other. @see IAlgorithm::validateInputs
  std::map<std::string, std::string> validateInputs() override;
  /// Check if input workspace is a group. @see Algorithm::checkGroups
  bool checkGroups() override;

private:
  /// Algorithm initialisation
  void init() override;
  /// Algorithm execution for single MatrixWorkspaces
  void exec() override;
  /// Algorithm execution for WorkspaceGroups
  bool processGroups() override;
  /// Check file validity
  void checkFile(const std::string filename);
  /// Write the data
  void data();
  /// Print a double value to file
  void outputval(double val);
  /// Write a string value
  bool writeString(bool write, std::string s);
  /// Print a string value to file
  void outputval(std::string val);
  /// Retrieve sample log information
  std::string sampleInfo(const std::string &logName);
  /// Write one header line
  void writeInfo(const std::string logName, const std::string logValue = "");
  /// Write header
  void header();
  /// Determine the separator
  void separator();
  /// Separator
  char m_sep{'\0'};
  /// Filename
  std::string m_filename;
  /// File extension
  std::string m_ext;
  /// Input workspace
  API::MatrixWorkspace_const_sptr m_ws;
  /// Input workspace group
  std::vector<API::MatrixWorkspace_const_sptr> m_group;
  /// Names of the workspaces in a group
  std::vector<std::string> m_wsName;
  /// The output file stream
  std::ofstream m_file;
};
} // namespace DataHandling
} // namespace Mantid

#endif /*  MANTID_DATAHANDLING_SaveReflectometryAscii_H_  */
