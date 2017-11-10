#ifndef MANTID_CURVEFITTING_HISTOGRAMDOMAINCREATOR_H_
#define MANTID_CURVEFITTING_HISTOGRAMDOMAINCREATOR_H_

#include "MantidAPI/Workspace_fwd.h"
#include "MantidCurveFitting/IMWDomainCreator.h"
#include "MantidKernel/System.h"

namespace Mantid {

namespace API {
class FunctionDomain1DHistogram;
}

namespace CurveFitting {

/** HistogramDomainCreator:

    HistogramDomainCreator creates a FunctionDomain1DHistogram which is
    used with IFunction1D.

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

class DLLExport HistogramDomainCreator : public IMWDomainCreator {
public:
  HistogramDomainCreator(Kernel::IPropertyManager &manager,
                         const std::string &workspacePropertyName);

  void createDomain(boost::shared_ptr<API::FunctionDomain> &domain,
                    boost::shared_ptr<API::FunctionValues> &values,
                    size_t i0 = 0) override;
  boost::shared_ptr<API::Workspace> createOutputWorkspace(
      const std::string &baseName, API::IFunction_sptr function,
      boost::shared_ptr<API::FunctionDomain> domain,
      boost::shared_ptr<API::FunctionValues> values,
      const std::string &outputWorkspacePropertyName) override;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_HISTOGRAMDOMAINCREATOR_H_ */
