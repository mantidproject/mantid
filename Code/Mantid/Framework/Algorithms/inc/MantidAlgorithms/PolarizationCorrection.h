#ifndef MANTID_ALGORITHMS_POLARIZATIONCORRECTION_H_
#define MANTID_ALGORITHMS_POLARIZATIONCORRECTION_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>

namespace Mantid {
namespace API {
class WorkspaceGroup;
class MatrixWorkspace;
}
namespace Algorithms {

/** PolarizationCorrection : Algorithm to perform polarisation corrections on
 multi-period group workspaces.

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
class DLLExport PolarizationCorrection : public API::Algorithm {
public:
  PolarizationCorrection();
  virtual ~PolarizationCorrection();
  virtual const std::string name() const;
  virtual int version() const;
  virtual const std::string category() const;
  const std::string summary() const;

private:
  void init();
  void exec();
  boost::shared_ptr<Mantid::API::MatrixWorkspace> execPolynomialCorrection(
      boost::shared_ptr<Mantid::API::MatrixWorkspace> &input,
      const std::vector<double> &coefficients);
  boost::shared_ptr<Mantid::API::WorkspaceGroup>
  execPA(boost::shared_ptr<Mantid::API::WorkspaceGroup> inWS);
  boost::shared_ptr<Mantid::API::WorkspaceGroup>
  execPNR(boost::shared_ptr<Mantid::API::WorkspaceGroup> inWS);
  boost::shared_ptr<Mantid::API::MatrixWorkspace>
  add(boost::shared_ptr<Mantid::API::MatrixWorkspace> &lhsWS,
      const double &rhs);
  boost::shared_ptr<Mantid::API::MatrixWorkspace>
  copyShapeAndFill(boost::shared_ptr<Mantid::API::MatrixWorkspace> &base,
                   const double &value);
  bool isPropertyDefault(const std::string &propertyName) const;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_POLARIZATIONCORRECTION_H_ */
