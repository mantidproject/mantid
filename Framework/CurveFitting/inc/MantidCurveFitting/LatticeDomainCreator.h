// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_LATTICEDOMAINCREATOR_H_
#define MANTID_CURVEFITTING_LATTICEDOMAINCREATOR_H_

#include "MantidAPI/IDomainCreator.h"
#include "MantidAPI/IPeaksWorkspace_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace CurveFitting {

/** LatticeDomainCreator

  Domain creator for LatticeDomain, which processes IPeaksWorkspace or
  an ITableWorkspace with certain columns (HKL, d).

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 15/04/2015
*/
class DLLExport LatticeDomainCreator : public API::IDomainCreator {
public:
  LatticeDomainCreator(Kernel::IPropertyManager *manager,
                       const std::string &workspacePropertyName,
                       DomainType domainType = Simple);

  void createDomain(boost::shared_ptr<API::FunctionDomain> &domain,
                    boost::shared_ptr<API::FunctionValues> &values,
                    size_t i0) override;

  API::Workspace_sptr createOutputWorkspace(
      const std::string &baseName, API::IFunction_sptr function,
      boost::shared_ptr<API::FunctionDomain> domain,
      boost::shared_ptr<API::FunctionValues> values,
      const std::string &outputWorkspacePropertyName) override;

  size_t getDomainSize() const override;

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
