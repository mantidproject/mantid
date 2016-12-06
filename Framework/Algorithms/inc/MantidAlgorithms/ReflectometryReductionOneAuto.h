#ifndef MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTO_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTO_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidKernel/System.h"

#include <boost/optional.hpp>

namespace Mantid {
namespace Algorithms {

/** ReflectometryReductionOneAuto : Algorithm to run ReflectometryReductionOne,
attempting to pick instrument parameters for
 * missing properties.

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
class DLLExport ReflectometryReductionOneAuto
    : public API::DataProcessorAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

  // For (multiperiod) workspace groups
  bool checkGroups() override;
  bool processGroups() override;

private:
  void init() override;
  void exec() override;
  template <typename T> boost::optional<T> isSet(std::string propName) const;
  Mantid::API::Workspace_sptr
  sumOverTransmissionGroup(Mantid::API::WorkspaceGroup_sptr &transGroup);

  std::string pNRLabel() const { return "PNR"; }
  std::string pALabel() const { return "PA"; }
  std::string crhoLabel() const { return "CRho"; }
  std::string cppLabel() const { return "CPp"; }
  std::string cAlphaLabel() const { return "CAlpha"; }
  std::string cApLabel() const { return "CAp"; }
  std::string noPolarizationCorrectionMode() const { return "None"; }
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTO_H_ */
