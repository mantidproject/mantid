#ifndef MANTID_MDALGORITHMS_SMOOTHMD_H_
#define MANTID_MDALGORITHMS_SMOOTHMD_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace API {
class IMDHistoWorkspace;
}
namespace MDAlgorithms {

DLLExport std::vector<double> gaussianKernel(const double fwhm);
DLLExport std::vector<double> normaliseKernel(std::vector<double> kernel);
DLLExport std::vector<double>
renormaliseKernel(std::vector<double> kernel,
                  const std::vector<bool> &validity);

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

  File change history is stored at:
  <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport SmoothMD : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  std::map<std::string, std::string> validateInputs() override;

  boost::shared_ptr<Mantid::API::IMDHistoWorkspace> hatSmooth(
      boost::shared_ptr<const Mantid::API::IMDHistoWorkspace> toSmooth,
      const std::vector<double> &widthVector,
      boost::optional<boost::shared_ptr<const Mantid::API::IMDHistoWorkspace>>
          weightingWS);

  boost::shared_ptr<Mantid::API::IMDHistoWorkspace> gaussianSmooth(
      boost::shared_ptr<const Mantid::API::IMDHistoWorkspace> toSmooth,
      const std::vector<double> &widthVector,
      boost::optional<boost::shared_ptr<const Mantid::API::IMDHistoWorkspace>>
          weightingWS);

private:
  void init() override;
  void exec() override;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_SMOOTHMD_H_ */
