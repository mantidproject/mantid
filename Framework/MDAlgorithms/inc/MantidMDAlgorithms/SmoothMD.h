#ifndef MANTID_MDALGORITHMS_SMOOTHMD_H_
#define MANTID_MDALGORITHMS_SMOOTHMD_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>

namespace Mantid {
namespace API {
class IMDHistoWorkspace;
}
namespace MDAlgorithms {

/** SmoothMD : Algorithm for smoothing MDHistoWorkspaces

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
class DLLExport SmoothMD : public API::Algorithm {
public:
  SmoothMD();
  virtual ~SmoothMD();

  virtual const std::string name() const;
  virtual int version() const;
  virtual const std::string category() const;
  virtual const std::string summary() const;
  std::map<std::string, std::string> validateInputs();

  boost::shared_ptr<Mantid::API::IMDHistoWorkspace> hatSmooth(
      boost::shared_ptr<const Mantid::API::IMDHistoWorkspace> toSmooth,
      const std::vector<int> &widthVector,
      boost::optional<boost::shared_ptr<const Mantid::API::IMDHistoWorkspace>>
          weightingWS);

private:
  void init();
  void exec();
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_SMOOTHMD_H_ */
