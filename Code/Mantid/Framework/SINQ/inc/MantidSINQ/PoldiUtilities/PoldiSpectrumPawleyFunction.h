#ifndef MANTID_SINQ_POLDISPECTRUMPAWLEYFUNCTION_H_
#define MANTID_SINQ_POLDISPECTRUMPAWLEYFUNCTION_H_

#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/PoldiSpectrumDomainFunction.h"
#include "MantidAPI/IPawleyFunction.h"

namespace Mantid {
namespace Poldi {

/** PoldiSpectrumPawleyFunction : TODO: DESCRIPTION

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
class MANTID_SINQ_DLL PoldiSpectrumPawleyFunction
    : public PoldiSpectrumDomainFunction {
public:
  PoldiSpectrumPawleyFunction();
  virtual ~PoldiSpectrumPawleyFunction() {}

  std::string name() const { return "PoldiSpectrumPawleyFunction"; }

  void
  setMatrixWorkspace(boost::shared_ptr<const API::MatrixWorkspace> workspace,
                     size_t wi, double startX, double endX);

  void function1DSpectrum(const API::FunctionDomain1DSpectrum &domain,
                          API::FunctionValues &values) const;
  void functionDeriv1DSpectrum(const API::FunctionDomain1DSpectrum &domain,
                               API::Jacobian &jacobian);
  void poldiFunction1D(const std::vector<int> &indices,
                       const API::FunctionDomain1D &domain,
                       API::FunctionValues &values) const;

  API::IPawleyFunction_sptr getPawleyFunction() const;

protected:
  void beforeDecoratedFunctionSet(const API::IFunction_sptr &fn);

  API::IPawleyFunction_sptr m_pawleyFunction;
};

} // namespace Poldi
} // namespace Mantid

#endif /* MANTID_SINQ_POLDISPECTRUMPAWLEYFUNCTION_H_ */
