#ifndef MANTID_DATAHANDLING_LOADASCII_H_
#define MANTID_DATAHANDLING_LOADASCII_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFileLoader.h"

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

Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport LoadAscii : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  /// Default constructor
  LoadAscii();
  /// The name of the algorithm
  virtual const std::string name() const { return "LoadAscii"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Loads data from a text file and stores it in a 2D workspace "
           "(Workspace2D class).";
  }

  /// The version number
  virtual int version() const { return 1; }
  /// The category
  virtual const std::string category() const { return "DataHandling\\Text"; }
  /// Returns a confidence value that this algorithm can load a file
  virtual int confidence(Kernel::FileDescriptor &descriptor) const;

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
  int splitIntoColumns(std::list<std::string> &columns,
                       const std::string &str) const;
  /// Fill the given vector with the data values
  void fillInputValues(std::vector<double> &values,
                       const std::list<std::string> &columns) const;

  /// The column separator
  std::string m_columnSep;

private:
  /// Declare properties
  void init();
  /// Execute the algorithm
  void exec();

  /// Map the separator options to their string equivalents
  std::map<std::string, std::string> m_separatorIndex;
};

} // namespace DataHandling
} // namespace Mantid

#endif /*  MANTID_DATAHANDLING_LOADASCII_H_  */
