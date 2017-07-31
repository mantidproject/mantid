#ifndef MANTID_DATAHANDLING_LOADSESANS_H_
#define MANTID_DATAHANDLING_LOADSESANS_H_

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/DllConfig.h"

#include <unordered_map>
#include <utility>

typedef std::vector<double> Column;
typedef std::unordered_map<std::string, Column> ColumnMap;

namespace Mantid {
namespace DataHandling {

/** LoadSESANS : Load a workspace in the SESANS file format

        Required properties:
        <UL>
        <LI> Filename - The path to the file</LI>
        <LI> OutputWorkspace - The name of the output workspace</LI>
        </UL>

        @author Joseph Ramsay, ISIS
        @date 20/07/2017

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_DATAHANDLING_DLL LoadSESANS
    : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  const std::string name() const override;
  const std::string summary() const override;
  int version() const override;
  const std::string category() const override;
  int confidence(Kernel::FileDescriptor &descriptor) const override;

private:
  // Private constants
  const std::vector<std::string> mandatoryAttributes{
      "FileFormatVersion", "DataFileTitle",       "Sample",
      "Thickness",         "Thickness_unit",      "Theta_zmax",
      "Theta_zmax_unit",   "Theta_ymax",          "Theta_ymax_unit",
      "Orientation",       "SpinEchoLength_unit", "Depolarisation_unit",
      "Wavelength_unit"};
  const std::vector<std::string> mandatoryColumnHeaders{
      "SpinEchoLength", "Depolarisation", "Depolarisation_error", "Wavelength"};
  const std::vector<std::string> fileExtensions{".ses", ".SES", ".sesans",
                                                ".SESANS"};

  // Private functions
  void init() override;
  void exec() override;

  void consumeHeaders(std::ifstream &infile, std::string &line, int &lineNum);
  ColumnMap consumeData(std::ifstream &infile, std::string &line, int &lineNum);
  std::pair<std::string, std::string> splitHeader(const std::string &line,
                                                  const int &lineNum);
  void checkMandatoryHeaders();

  void throwFormatError(const std::string &line, const std::string &message,
                        const int &lineNum);

  API::MatrixWorkspace_sptr makeWorkspace(ColumnMap columns);
  Column calculateYValues(const Column &depolarisation,
                          const Column &wavelength) const;
  Column calculateEValues(const Column &error, const Column &yValues,
                          const Column &wavelength) const;

  // Private helper functions
  static bool space(const char &c);
  static bool notSpace(const char &c);
  static std::vector<std::string> split(const std::string &str);
  static std::string repeatAndJoin(const std::string &str,
                                   const std::string &delim, const int &n);

  // Private members
  std::unordered_map<std::string, std::string> attributes;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADSESANS_H_ */
