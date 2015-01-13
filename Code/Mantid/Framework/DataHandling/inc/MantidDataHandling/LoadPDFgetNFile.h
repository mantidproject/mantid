#ifndef MANTID_DATAHANDLING_LOADPDFGETNFILE_H_
#define MANTID_DATAHANDLING_LOADPDFGETNFILE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid {
namespace DataHandling {

/** LoadPDFgetNFile : TODO: DESCRIPTION

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport LoadPDFgetNFile
    : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  LoadPDFgetNFile();
  virtual ~LoadPDFgetNFile();

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "LoadPDFgetNFile"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Types of PDFgetN data files include .sqa, .sq, .gr, and etc.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }

  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const {
    return "Diffraction;DataHandling\\Text";
  }

private:
  /// Implement abstract Algorithm methods
  void init();
  /// Implement abstract Algorithm methods
  void exec();
  /// Returns a confidence value that this algorithm can load a file
  virtual int confidence(Kernel::FileDescriptor &descriptor) const;

  /// Parse PDFgetN data file
  void parseDataFile(std::string filename);

  /// Check whether a string starts from a specified sub-string
  bool startsWith(const std::string &s, const std::string &header) const;

  /// Parse column name line staring with \#L
  void parseColumnNameLine(std::string line);

  /// Parse data line
  void parseDataLine(std::string line);

  /// Output data workspace
  DataObjects::Workspace2D_sptr outWS;

  /// Data structure to hold input:  Size = Number of columns in input file
  std::vector<std::vector<double>> mData;

  /// Names of the columns of the data
  std::vector<std::string> mColumnNames;

  /// Generate output workspace
  void generateDataWorkspace();

  /// Set X and Y axis unit and lebel
  void setUnit(DataObjects::Workspace2D_sptr ws);
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADPDFGETNFILE_H_ */
