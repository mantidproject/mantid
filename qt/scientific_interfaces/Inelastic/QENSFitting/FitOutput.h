// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "IFitOutput.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {
using namespace MantidWidgets;

/*
   FitOutput - Specifies an interface for updating, querying and
   accessing the raw data in Tabs
*/
class MANTIDQT_INELASTIC_DLL FitOutput : public IFitOutput {
public:
  FitOutput();
  bool isSpectrumFit(FitDomainIndex index) const override;

  std::unordered_map<std::string, ParameterValue> getParameters(FitDomainIndex index) const override;

  std::optional<ResultLocationNew> getResultLocation(FitDomainIndex index) const override;
  std::vector<std::string> getResultParameterNames() const override;
  Mantid::API::WorkspaceGroup_sptr getLastResultWorkspace() const override;
  Mantid::API::WorkspaceGroup_sptr getLastResultGroup() const override;

  void clear() override;
  bool isEmpty() const override;
  void addOutput(const Mantid::API::WorkspaceGroup_sptr &resultGroup, Mantid::API::ITableWorkspace_sptr parameterTable,
                 const Mantid::API::WorkspaceGroup_sptr &resultWorkspace, FitDomainIndex fitDomainIndex) override;

private:
  std::weak_ptr<Mantid::API::WorkspaceGroup> m_resultGroup;
  std::weak_ptr<Mantid::API::WorkspaceGroup> m_resultWorkspace;
  std::unordered_map<size_t, std::unordered_map<std::string, ParameterValue>> m_parameters;
  std::unordered_map<size_t, ResultLocationNew> m_outputResultLocations;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
