#ifndef MANTID_MDALGORITHMS_SetMDFrame_H_
#define MANTID_MDALGORITHMS_SetMDFrame_H_

#include "MantidMDAlgorithms/DllConfig.h"
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/MDGeometry/MDFrame.h"

namespace Mantid {
namespace MDAlgorithms {

/** SetMDFrame : This algorithm changes the MDFrame stored alongside the
    dimension of MDWorkspaes.The algorithm should primarily be used to
    introduce the correct MDFrame type to workspaces based on legacy files.

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
class MANTID_MDALGORITHMS_DLL SetMDFrame : public API::Algorithm {
public:
  static const std::string mdFrameSpecifier;
  SetMDFrame();
  virtual ~SetMDFrame();

  virtual const std::string name() const;
  virtual int version() const;
  virtual const std::string category() const;
  virtual const std::string summary() const;
  virtual std::map<std::string, std::string> validateInputs();

private:
  void init();
  void exec();
  Mantid::Geometry::MDFrame_uptr
  createMDFrame(const std::string &frameSelection,
                const Mantid::Geometry::MDFrame &oldFrame) const;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_SetMDFrame_H_ */