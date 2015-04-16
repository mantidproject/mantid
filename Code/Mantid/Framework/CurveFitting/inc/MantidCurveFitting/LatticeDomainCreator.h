#ifndef MANTID_CURVEFITTING_LATTICEDOMAINCREATOR_H_
#define MANTID_CURVEFITTING_LATTICEDOMAINCREATOR_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IDomainCreator.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/IPeaksWorkspace.h"

namespace Mantid {
namespace CurveFitting {

/** LatticeDomainCreator

  Domain creator for LatticeDomain, which processes IPeaksWorkspace or
  an ITableWorkspace with certain columns (HKL, d).

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 15/04/2015

  Copyright © 2015 PSI-NXMM

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
class DLLExport LatticeDomainCreator : public API::IDomainCreator {
public:
  LatticeDomainCreator(Kernel::IPropertyManager *manager,
                       const std::string &workspacePropertyName,
                       DomainType domainType = Simple);

  virtual ~LatticeDomainCreator() {}

  virtual void createDomain(boost::shared_ptr<API::FunctionDomain> &domain,
                            boost::shared_ptr<API::FunctionValues> &values,
                            size_t i0);

  virtual API::Workspace_sptr createOutputWorkspace(
          const std::string &baseName,
          API::IFunction_sptr function,
          boost::shared_ptr<API::FunctionDomain> domain,
          boost::shared_ptr<API::FunctionValues> values,
          const std::string &outputWorkspacePropertyName);

  virtual size_t getDomainSize() const;

protected:
  void setWorkspaceFromPropertyManager();
  void
  createDomainFromPeaksWorkspace(const API::IPeaksWorkspace_sptr &workspace,
                                 boost::shared_ptr<API::FunctionDomain> &domain,
                                 boost::shared_ptr<API::FunctionValues> &values,
                                 size_t i0);

  void createDomainFromPeakTable(const API::ITableWorkspace_sptr &workspace,
                                 boost::shared_ptr<API::FunctionDomain> &domain,
                                 boost::shared_ptr<API::FunctionValues> &values,
                                 size_t i0);

  std::string m_workspacePropertyName;
  API::Workspace_sptr m_workspace;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_LATTICEDOMAINCREATOR_H_ */
