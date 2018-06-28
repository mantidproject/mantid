#ifndef MANTID_DATAHANDLING_CREATEPOLARIZATIONEFFICIENCIESBASE_H_
#define MANTID_DATAHANDLING_CREATEPOLARIZATIONEFFICIENCIESBASE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>

namespace Mantid {
namespace DataHandling {

/** CreatePolarizationEfficienciesBase - the base class for algorithms
 that create polarization efficiency workspaces:

   - CreatePolarizationEfficiencies
   - JoinISISPolarizationEfficiencies
   - LoadISISPolarizationEfficiencies

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
class DLLExport CreatePolarizationEfficienciesBase : public API::Algorithm {
public:
  const std::string category() const override;

protected:
  void initOutputWorkspace();
  std::vector<std::string>
  getNonDefaultProperties(std::vector<std::string> const &props) const;

  /// Names of the efficiency properties
  static std::string const Pp;
  static std::string const Ap;
  static std::string const Rho;
  static std::string const Alpha;
  static std::string const P1;
  static std::string const P2;
  static std::string const F1;
  static std::string const F2;

private:
  void exec() override;
  /// Create the output workspace with efficiencies
  /// @param labels :: Names of the efficiencies to create
  virtual API::MatrixWorkspace_sptr
  createEfficiencies(std::vector<std::string> const &labels) = 0;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_CREATEPOLARIZATIONEFFICIENCIESBASE_H_ */
