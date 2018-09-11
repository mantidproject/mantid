#ifndef MANTID_DATAHANDLING_LOADTBL_H_
#define MANTID_DATAHANDLING_LOADTBL_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFileLoader.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid {
namespace DataHandling {
/**
Loads a table workspace from an ascii file in reflectometry tbl format. Rows
must be no longer than 17 cells.

Copyright &copy; 2007-2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport LoadTBL : public API::IFileLoader<Kernel::FileDescriptor> {
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
  const std::vector<std::string> seeAlso() const override {
    return {"SaveTBL"};
  }
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
  size_t getCells(std::string line, std::vector<std::string> &cols,
                  size_t expectedCommas, bool isOldTBL) const;
  /// count the number of commas in the line
  size_t countCommas(std::string line) const;
  /// find all pairs of quotes in the line
  size_t findQuotePairs(std::string line,
                        std::vector<std::vector<size_t>> &quoteBounds) const;
  /// Parse more complex CSV, used when the data involves commas in the data and
  /// quoted values
  void csvParse(std::string line, std::vector<std::string> &cols,
                std::vector<std::vector<size_t>> &quoteBounds,
                size_t expectedCommas) const;
};

} // namespace DataHandling
} // namespace Mantid

#endif /*  MANTID_DATAHANDLING_LOADTBL_H_  */
