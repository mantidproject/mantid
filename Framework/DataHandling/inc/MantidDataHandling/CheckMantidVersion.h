#ifndef MANTID_DATAHANDLING_CHECKMANTIDVERSION_H_
#define MANTID_DATAHANDLING_CHECKMANTIDVERSION_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataHandling {

/** CheckMantidVersion : Checks if the current version of Mantid is the most
  recent

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport CheckMantidVersion : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

protected:
  virtual std::string getVersionsFromGitHub(const std::string &url);
  virtual std::string getCurrentVersion() const;

private:
  void init() override;
  void exec() override;

  std::string cleanVersionTag(const std::string &versionTag) const;
  std::vector<int> splitVersionString(const std::string &versionString) const;
  bool isVersionMoreRecent(const std::string &localVersion,
                           const std::string &gitHubVersion) const;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_CHECKMANTIDVERSION_H_ */