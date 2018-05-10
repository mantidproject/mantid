#ifndef MANTID_CRYSTAL_SETSPECIALCOORDINATES_H_
#define MANTID_CRYSTAL_SETSPECIALCOORDINATES_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/System.h"
#include <string>
#include <vector>

namespace Mantid {
namespace Crystal {

/** SetSpecialCoordinates :
 *

  Set the special coordinates on an IMDWorspace or peaksworkspace. Also print
 out any existing special coordinates.

  Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport SetSpecialCoordinates : public API::Algorithm {
public:
  SetSpecialCoordinates();

  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Set or overwrite any Q3D special coordinates.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"ConvertToMD", "ConvertToDiffractionMDWorkspace"};
  }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
  std::vector<std::string> m_specialCoordinatesNames;
  using SpecialCoordinatesNameMap =
      std::map<std::string, Mantid::Kernel::SpecialCoordinateSystem>;
  SpecialCoordinatesNameMap m_specialCoordinatesMap;
  static const std::string QLabOption();
  static const std::string QSampleOption();
  static const std::string HKLOption();
  bool writeCoordinatesToMDEventWorkspace(
      Mantid::API::Workspace_sptr inWS,
      Mantid::Kernel::SpecialCoordinateSystem coordinateSystem);
  bool writeCoordinatesToMDHistoWorkspace(
      Mantid::API::Workspace_sptr inWS,
      Mantid::Kernel::SpecialCoordinateSystem coordinateSystem);
  bool writeCoordinatesToPeaksWorkspace(
      Mantid::API::Workspace_sptr inWS,
      Mantid::Kernel::SpecialCoordinateSystem coordinateSystem);
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_SETSPECIALCOORDINATES_H_ */
