#ifndef MANTID_CRYSTAL_SETSPECIALCOORDINATES_H_
#define MANTID_CRYSTAL_SETSPECIALCOORDINATES_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDWorkspace.h"
#include <vector>
#include <string>

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
  virtual ~SetSpecialCoordinates();

  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Set or overwrite any Q3D special coordinates.";
  }

  virtual int version() const;
  virtual const std::string category() const;

private:
  void init();
  void exec();
  std::vector<std::string> m_specialCoordinatesNames;
  typedef std::map<std::string, Mantid::API::SpecialCoordinateSystem>
      SpecialCoordinatesNameMap;
  SpecialCoordinatesNameMap m_specialCoordinatesMap;
  static const std::string QLabOption();
  static const std::string QSampleOption();
  static const std::string HKLOption();
  bool writeCoordinatesToMDEventWorkspace(
      Mantid::API::Workspace_sptr inWS,
      Mantid::API::SpecialCoordinateSystem coordinateSystem);
  bool writeCoordinatesToMDHistoWorkspace(
      Mantid::API::Workspace_sptr inWS,
      Mantid::API::SpecialCoordinateSystem coordinateSystem);
  bool writeCoordinatesToPeaksWorkspace(
      Mantid::API::Workspace_sptr inWS,
      Mantid::API::SpecialCoordinateSystem coordinateSystem);
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_SETSPECIALCOORDINATES_H_ */
