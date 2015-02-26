#ifndef MANTID_DATAHANDLING_LOADGSASINSTRUMENTFILE_H_
#define MANTID_DATAHANDLING_LOADGSASINSTRUMENTFILE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Poco {
namespace XML {
class Document;
class Element;
}
}

namespace Mantid {
namespace DataHandling {

/** LoadGSASInstrumentFile : Load GSAS instrument file to TableWorkspace(s)

  Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport LoadGSASInstrumentFile : public API::Algorithm {
public:
  LoadGSASInstrumentFile();
  virtual ~LoadGSASInstrumentFile();

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "LoadGSASInstrumentFile"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Load parameters from a GSAS Instrument file.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }

  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Diffraction"; }

private:
  /// Implement abstract Algorithm methods
  void init();
  /// Implement abstract Algorithm methods
  void exec();

  /// Load file to a vector of strings
  void loadFile(std::string filename, std::vector<std::string> &lines);

  /// Get Histogram type
  std::string getHistogramType(const std::vector<std::string> &lines);

  /// Get Number of banks
  size_t getNumberOfBanks(const std::vector<std::string> &lines);

  /// Scan imported file for bank information
  void scanBanks(const std::vector<std::string> &lines,
                 std::vector<size_t> &bankStartIndex);

  /// Parse bank in file to a map
  void parseBank(std::map<std::string, double>& parammap, const std::vector<std::string>& lines, size_t bankid, size_t startlineindex);

  /// Find first INS line at or after lineIndex
  size_t findINSPRCFLine(const std::vector<std::string>& lines, size_t lineIndex, double& param1, double& param2, double& param3, double& param4);

  /// Generate output workspace
  DataObjects::TableWorkspace_sptr genTableWorkspace(
      std::map<size_t, std::map<std::string, double>> bankparammap);
};

} // namespace DataHandling
} // namespace Mantid

#endif  /* MANTID_DATAHANDLING_LOADGSASINSTRUMENTFILE_H_ */
