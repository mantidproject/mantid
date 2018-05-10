#ifndef MANTID_DATAHANDLING_LOADSPICEASCII_H_
#define MANTID_DATAHANDLING_LOADSPICEASCII_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidKernel/System.h"

#include "MantidAPI/IFileLoader.h"

namespace Mantid {
namespace DataHandling {

/** LoadSpiceAscii : TODO: DESCRIPTION

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
class DLLExport LoadSpiceAscii : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"LoadSpice2D", "LoadSpiceXML2DDet"};
  }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;

  bool validateLogNamesType(const std::vector<std::string> &floatlognames,
                            const std::vector<std::string> &intlognames,
                            const std::vector<std::string> &strlognames);

  /// Parse SPICE Ascii file to dictionary
  void parseSPICEAscii(const std::string &filename,
                       std::vector<std::vector<std::string>> &datalist,
                       std::vector<std::string> &titles,
                       std::map<std::string, std::string> &runinfodict);

  /// Create data workspace
  API::ITableWorkspace_sptr
  createDataWS(const std::vector<std::vector<std::string>> &datalist,
               const std::vector<std::string> &titles);

  /// Create run information workspace
  API::MatrixWorkspace_sptr
  createRunInfoWS(std::map<std::string, std::string> runinfodict,
                  std::vector<std::string> &floatlognamelist,
                  std::vector<std::string> &intlognamelist,
                  std::vector<std::string> &strlognamelist,
                  bool ignoreunlisted);

  /// Convert input date string to mantid date string
  std::string processDateString(const std::string &rawdate,
                                const std::string &dateformat);

  /// Convert input time string to mantid time string
  std::string processTimeString(const std::string &rawtime,
                                const std::string &timeformat);

  /// Set up run start time
  void setupRunStartTime(API::MatrixWorkspace_sptr runinfows,
                         const std::vector<std::string> &datetimeprop);

  /// Add property to workspace
  template <typename T>
  void addProperty(API::MatrixWorkspace_sptr ws, const std::string &pname,
                   T pvalue);
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADSPICEASCII_H_ */
