#ifndef MANTID_CUSTOMINTERFACES_ALCLATESTFILEFINDER_H_
#define MANTID_CUSTOMINTERFACES_ALCLATESTFILEFINDER_H_

#include "MantidQtCustomInterfaces/DllConfig.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/** ALCLatestFileFinder : Utility to find most recent file in a directory

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTIDQT_CUSTOMINTERFACES_DLL ALCLatestFileFinder {
public:
  /// Constructor - takes filename of first run
  explicit ALCLatestFileFinder(const std::string &firstRunFile)
      : m_firstRunFileName(firstRunFile){};

  /// Find most recent file in same directory as first run
  std::string getMostRecentFile() const;

protected:
  /// Check validity of filename
  bool isValid(const std::string &path) const;

private:
  /// Filename of first run
  std::string m_firstRunFileName;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_ALCLATESTFILEFINDER_H_ */