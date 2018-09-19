#ifndef MANTID_ALGORITHMS_POLARIZATIONCORRECTIONFREDRIKZE_H_
#define MANTID_ALGORITHMS_POLARIZATIONCORRECTIONFREDRIKZE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace API {
class WorkspaceGroup;
class MatrixWorkspace;
} // namespace API
namespace Algorithms {

/** PolarizationCorrectionFredrikze : Algorithm to perform polarisation
 corrections on
 multi-period group workspaces that implements the Fredrikze (Dutch) method.
 Fredrikze, H, et al. “Calibration of a polarized neutron reflectometer” Physica
 B 297 (2001)

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
class DLLExport PolarizationCorrectionFredrikze : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"PolarizationEfficiencyCor"};
  }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  boost::shared_ptr<Mantid::API::MatrixWorkspace>
  getEfficiencyWorkspace(const std::string &label);
  boost::shared_ptr<Mantid::API::WorkspaceGroup>
  execPA(boost::shared_ptr<Mantid::API::WorkspaceGroup> inWS);
  boost::shared_ptr<Mantid::API::WorkspaceGroup>
  execPNR(boost::shared_ptr<Mantid::API::WorkspaceGroup> inWS);
  boost::shared_ptr<Mantid::API::MatrixWorkspace>
  add(boost::shared_ptr<Mantid::API::MatrixWorkspace> &lhsWS,
      const double &rhs);
  boost::shared_ptr<Mantid::API::MatrixWorkspace>
  multiply(boost::shared_ptr<Mantid::API::MatrixWorkspace> &lhsWS,
           const double &rhs);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_POLARIZATIONCORRECTIONFREDRIKZE_H_ */
