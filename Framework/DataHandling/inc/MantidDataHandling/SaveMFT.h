#ifndef MANTID_DATAHANDLING_SaveMFT_H_
#define MANTID_DATAHANDLING_SaveMFT_H_

#include "MantidAPI/Algorithm.h"

#include <iostream>
#include <fstream>
#include <string>

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
class DLLExport SaveMFT : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "SaveMFT"; }
  /// Algorithm's version
  int version() const override { return 1; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Saves a 2D workspace to a ascii file of format mft";
  }
  /// Algorithm's with similar purpose
  const std::vector<std::string> seeAlso() const override {
    return {"SaveAscii"};
  }

private:
  /// Algorithm initialisation
  void init() override;
  /// Algorithm execution
  void exec() override;
  /// Write the data
  void data();
  /// Print data to file
  void outputval(double val);
  /// Retrieve sample log information
  std::string sampleInfo(const std::string &logName);
  /// Write one header line
  void writeInfo(const std::string logName, const std::string logValue = "");
  /// Write header
  void header();
  /// Number of data
  size_t m_length{0};
  /// Input workspace
  API::MatrixWorkspace_const_sptr m_ws;
  /// The output file stream
  std::ofstream m_file;
};
} // namespace DataHandling
} // namespace Mantid

#endif /*  MANTID_DATAHANDLING_SaveMFT_H_  */
