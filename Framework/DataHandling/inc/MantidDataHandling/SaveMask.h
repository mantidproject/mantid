#ifndef MANTID_DATAHANDLING_SAVEMASKINGTOFILE_H_
#define MANTID_DATAHANDLING_SAVEMASKINGTOFILE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataHandling {

/** SaveMaskingToFile : TODO: DESCRIPTION

  @date 2011-11-09

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport SaveMask : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "SaveMask"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Save a MaskWorkspace/SpecialWorkspace2D to an XML file.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"SaveMask", "LoadMask"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "DataHandling\\Masking;Transforms\\Masking";
  }

private:
  /// Define input parameters
  void init() override;

  /// Main body to execute algorithm
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_SAVEMASKINGTOFILE_H_ */
