#ifndef MANTID_DATAHANDLING_SAVEFullprofRESOLUTION_H_
#define MANTID_DATAHANDLING_SAVEFullprofRESOLUTION_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid {
namespace DataHandling {

/** SaveFullprofResolution : TODO: DESCRIPTION

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
class DLLExport SaveFullprofResolution : public API::Algorithm {
public:
  SaveFullprofResolution();
  /// Algorithm's name
  const std::string name() const override { return "SaveFullprofResolution"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Save a Table workspace, which contains peak profile parameters' "
           "values, to a Fullprof resolution (.irf) file.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"SaveFocusedXYE"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Diffraction\\DataHandling;DataHandling\\Text";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  /// Write the header information

  /// Process properties
  void processProperties();

  /// Write out string for profile 10 .irf file
  std::string toProf10IrfString();

  /// Write out string for profile 9 .irf file
  std::string toProf9IrfString();

  /// Parse input workspace to map of parameters
  void parseTableWorkspace();

  /// Check wether a profile parameter map has the parameter
  bool has_key(std::map<std::string, double> profmap, std::string key);

  /// Map containing the name of value of each parameter required by .irf file
  std::map<std::string, double> m_profileParamMap;

  /// Input table workspace
  DataObjects::TableWorkspace_sptr m_profileTableWS;

  /// Output Irf file name
  std::string m_outIrfFilename;

  /// Bank to write
  int m_bankID;

  /// Profile number
  int m_fpProfileNumber;

  /// Append to existing file
  bool m_append;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_SAVEFullprofRESOLUTION_H_ */
